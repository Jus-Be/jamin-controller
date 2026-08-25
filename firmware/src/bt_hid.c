/**
 * bt_hid.c
 *
 * Bluetooth Classic HID Host for the NOOX QWERTY keyboard.
 * Optimized for the Raspberry Pi Pico 2 W (RP2350).
 */

#include "bt_hid.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "btstack.h"
#include <string.h>
#include <stdio.h>

#if __has_include("pico/bootsel_button.h")
#include "pico/bootsel_button.h"
#define read_bootsel_button() get_bootsel_button()
#else
#define read_bootsel_button() false
#endif

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/* ── Flash storage for paired address ──────────────────────────────────── */
#define FLASH_TARGET_OFFSET   (2u * 1024u * 1024u - 4096u)
#define FLASH_MAGIC_HI        0x4A    /* 'J' */
#define FLASH_MAGIC_LO        0xAC    /* paired-address marker */

static bd_addr_t s_paired_addr;
static bool      s_has_paired_addr = false;
static bool      s_force_discovery = true;
static bool      s_inquiry_active  = false;

/* ── Callback pointers ──────────────────────────────────────────────────── */
static bt_hid_key_down_cb_t s_on_key_down = NULL;
static bt_hid_key_up_cb_t   s_on_key_up   = NULL;

/* ── Previous key state for key-up detection ───────────────────────────── */
#define MAX_KEYS_HELD  6
static uint8_t s_prev_keycodes[MAX_KEYS_HELD];
static uint8_t s_prev_modifiers = 0;

/* ── HID Host connection handle ─────────────────────────────────────────── */
static uint16_t s_hid_cid = 0;

/* ── BTstack objects ────────────────────────────────────────────────────── */
static btstack_packet_callback_registration_t s_hci_event_cb_reg;
static btstack_timer_source_t                 s_led_timer;

/* ── Status LED ─────────────────────────────────────────────────────────── */
typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK_SLOW,
    LED_MODE_BLINK_FAST,
} led_mode_t;

static led_mode_t s_led_mode = LED_MODE_OFF;
static bool       s_led_mode_ready = false;
static bool       s_led_level = false;

static uint8_t hid_descriptor_storage[600]; // Buffer to store the keyboard's HID descriptor

#define LED_BLINK_SLOW_MS 500
#define LED_BLINK_FAST_MS 150

static bool led_available(void) {
    return true;
}

static void led_write(bool on)  {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);	
}

static void led_timer_handler(btstack_timer_source_t *ts) {
    (void)ts;
	
    switch (s_led_mode) {
        case LED_MODE_BLINK_SLOW:
            led_write(!s_led_level);
            btstack_run_loop_set_timer(&s_led_timer, LED_BLINK_SLOW_MS);
            btstack_run_loop_add_timer(&s_led_timer);
            break;
        case LED_MODE_BLINK_FAST:
            led_write(!s_led_level);
            btstack_run_loop_set_timer(&s_led_timer, LED_BLINK_FAST_MS);
            btstack_run_loop_add_timer(&s_led_timer);
            break;
        case LED_MODE_ON:
            led_write(true);
            break;
        case LED_MODE_OFF:
        default:
            led_write(false);
            break;
    }
}

static void set_led_mode(led_mode_t mode)
{
    if (!led_available()) return;
    if (s_led_mode_ready && s_led_mode == mode) return;

    btstack_run_loop_remove_timer(&s_led_timer);
    s_led_mode = mode;
    s_led_mode_ready = true;

    switch (s_led_mode) {
        case LED_MODE_ON:
            led_write(true);
            break;
        case LED_MODE_OFF:
            led_write(false);
            break;
        case LED_MODE_BLINK_SLOW:
            led_write(false);
            btstack_run_loop_set_timer(&s_led_timer, LED_BLINK_SLOW_MS);
            btstack_run_loop_add_timer(&s_led_timer);
            break;
        case LED_MODE_BLINK_FAST:
            led_write(false);
            btstack_run_loop_set_timer(&s_led_timer, LED_BLINK_FAST_MS);
            btstack_run_loop_add_timer(&s_led_timer);
            break;
    }
}

static void start_discovery(void)
{
    if (!s_inquiry_active) {
        printf("BT: starting inquiry");
        gap_inquiry_start(10);
        s_inquiry_active = true;
    }
    set_led_mode(LED_MODE_BLINK_FAST);
}

/* ── Report parsing ─────────────────────────────────────────────────────── */
static void process_hid_report(const uint8_t *report, uint16_t len)
{
    if (len < 8) return;

    uint8_t modifiers   = report[0];
    const uint8_t *keys = &report[2];

    for (uint8_t p = 0; p < MAX_KEYS_HELD; p++) {
        if (s_prev_keycodes[p] == 0) continue;
        bool still_held = false;
        for (uint8_t c = 0; c < MAX_KEYS_HELD; c++) {
            if (keys[c] == s_prev_keycodes[p]) { still_held = true; break; }
        }
        if (!still_held && s_on_key_up) {
            s_on_key_up(s_prev_keycodes[p], s_prev_modifiers);
        }
    }

    for (uint8_t c = 0; c < MAX_KEYS_HELD; c++) {
        if (keys[c] == 0) continue;
        bool is_new = true;
        for (uint8_t p = 0; p < MAX_KEYS_HELD; p++) {
            if (s_prev_keycodes[p] == keys[c]) { is_new = false; break; }
        }
        if (is_new && s_on_key_down) {
            s_on_key_down(keys[c], modifiers);
        }
    }

    memcpy(s_prev_keycodes, keys, MAX_KEYS_HELD);
    s_prev_modifiers = modifiers;
}

/* ── Persist / load paired address ─────────────────────────────────────── */
static void save_paired_addr(const bd_addr_t addr)
{
    static uint8_t buf[FLASH_SECTOR_SIZE] __attribute__((aligned(4)));
    memset(buf, 0xFF, sizeof(buf));
    memcpy(buf, addr, 6);
    buf[6] = FLASH_MAGIC_HI;
    buf[7] = FLASH_MAGIC_LO;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buf, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    printf("BT: Saved paired target hardware address to Flash.\n");
}

static bool load_paired_addr(bd_addr_t addr)
{
    const uint8_t *p = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    if (p[6] == FLASH_MAGIC_HI && p[7] == FLASH_MAGIC_LO) {
        memcpy(addr, p, 6);
        return true;
    }
    return false;
}

/* ── Complete BTstack packet handler ────────────────────────────────────── */
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	printf("BT: packet_handler\n");
    (void)channel; (void)size;

    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    
    switch (event_type) {
        case BTSTACK_EVENT_STATE:
			printf("BT: packet_handler - BTSTACK_EVENT_STATE\n");		
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                printf("BT: stack working\n");
                if (s_has_paired_addr && !s_force_discovery) {
                    printf("BT: reconnecting to stored device: %s\n", bd_addr_to_str(s_paired_addr));
                    set_led_mode(LED_MODE_BLINK_SLOW);
                    uint8_t status = hid_host_connect(s_paired_addr, HID_PROTOCOL_MODE_BOOT, &s_hid_cid);
                    if (status != ERROR_CODE_SUCCESS) {
                        printf("BT: connection initiation failed (0x%02x), forcing discovery.\n", status);
                        start_discovery();
                    }
                } else {
                    start_discovery();
                }
            }
            break;

        case GAP_EVENT_INQUIRY_RESULT: {
			printf("BT: packet_handler - GAP_EVENT_INQUIRY_RESULT\n");					
            bd_addr_t addr;
            gap_event_inquiry_result_get_bd_addr(packet, addr);
            uint32_t cod = gap_event_inquiry_result_get_class_of_device(packet);
            
            /* Major Class 0x0500 Peripheral, Minor Class 0x40 Keyboard = 0x002540 */
            if ((cod & 0x001F00) == 0x000500 && (cod & 0x0000C0) == 0x000040) {
                printf("BT: Found Keyboard! Address: %s, COD: 0x%06lx\n", bd_addr_to_str(addr), cod);
                gap_inquiry_stop();
                s_inquiry_active = false;
                
                memcpy(s_paired_addr, addr, 6);
                save_paired_addr(s_paired_addr);
                set_led_mode(LED_MODE_BLINK_SLOW);
                hid_host_connect(s_paired_addr, HID_PROTOCOL_MODE_BOOT, &s_hid_cid);
            }
            break;
        }

        case GAP_EVENT_INQUIRY_COMPLETE:
			printf("BT: packet_handler - GAP_EVENT_INQUIRY_COMPLETE\n");				
            s_inquiry_active = false;
            if (s_hid_cid == 0) {
                printf("BT: Inquiry finished. No keyboard matched. Retrying...\n");
                start_discovery();
            }
            break;

        case HCI_EVENT_HID_META:
			printf("BT: packet_handler - HCI_EVENT_HID_META\n");				
            switch (hci_event_hid_meta_get_subevent_code(packet)) {
                case HID_SUBEVENT_CONNECTION_OPENED:
                    if (hid_subevent_connection_opened_get_status(packet) == ERROR_CODE_SUCCESS) {
                        s_hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                        printf("BT: Connected successfully! CID: 0x%04x\n", s_hid_cid);
                        set_led_mode(LED_MODE_ON);
                    } else {
                        printf("BT: Connection failed (status 0x%02x). Retrying Discovery.\n", hid_subevent_connection_opened_get_status(packet));
                        s_hid_cid = 0;
                        start_discovery();
                    }
                    break;

                case HID_SUBEVENT_CONNECTION_CLOSED:
                    printf("BT: Disconnected (CID 0x%04x). Re-entering scan.\n", s_hid_cid);
                    s_hid_cid = 0;
                    start_discovery();
                    break;

                case HID_SUBEVENT_REPORT: {
                    uint16_t report_len = hid_subevent_report_get_report_len(packet);
                    const uint8_t *report_data = hid_subevent_report_get_report(packet);
                    process_hid_report(report_data, report_len);
                    break;
                }
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void bt_hid_init(bt_hid_key_down_cb_t on_down, bt_hid_key_up_cb_t on_up) {
    printf("BT: bt_hid_init\n");	
    s_on_key_down = on_down;
    s_on_key_up = on_up;

    // 1. Initialize core BTstack dependencies required for Classic HID Host
    l2cap_init();    
	printf("BT: bt_hid_init - l2cap_init done\n");
		
    #ifdef ENABLE_SEGURE_CONNECTIONS
    sm_init(); // Crucial for pairing exchange layers
	printf("BT: bt_hid_init - sm_init done\n");	
    #endif

    // 2. Initialize the HID Host Subsystem with descriptor cache
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
	printf("BT: bt_hid_init - hid_host_init done\n");		

    // 3. Register your event callbacks
    s_hci_event_cb_reg.callback = &packet_handler;
    hci_add_event_handler(&s_hci_event_cb_reg);
    hid_host_register_packet_handler(&packet_handler);
	printf("BT: bt_hid_init - hid_host_register_packet_handler done\n");	

    // 4. Handle paired addresses and flash overrides
    //s_has_paired_addr = load_paired_addr(s_paired_addr);
    printf("BT: Forcing clean pairing discovery...\n");
    s_force_discovery = true;

    btstack_run_loop_set_timer_handler(&s_led_timer, &led_timer_handler);
    printf("BT: bt_hid_init - btstack_run_loop_set_timer_handler\n");
	
	gap_discoverable_control(1);
	gap_connectable_control(1);	

	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);	
}