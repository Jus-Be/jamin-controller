/**
 * bt_hid.c
 *
 * Bluetooth Classic HID Host for the NOOX QWERTY keyboard.
 *
 * Uses BTstack (Pico SDK bundled version).
 *
 * Auto-pair flow:
 *   1. On first boot, enter GAP inquiry to discover nearby BT devices.
 *   2. Filter to HID keyboard COD (Class of Device).
 *   3. Initiate connection; accept SSP pairing.
 *   4. Save BD_ADDR to flash (last 4 KB of the 2 MB flash).
 *   5. Receive HID Boot Protocol keyboard reports via HID_SUBEVENT_REPORT.
 *
 * On subsequent boots:
 *   1. Load BD_ADDR from flash.
 *   2. Directly issue hid_host_connect().
 *   3. If connect fails, fall back to inquiry scan.
 *
 * HID Boot Protocol keyboard report (8 bytes):
 *   Byte 0 : modifier bitmask
 *   Byte 1 : reserved
 *   Bytes 2-7 : up to 6 simultaneous keycodes (0 = empty slot)
 */

#include "bt_hid.h"
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "btstack.h"
#include <string.h>
#include <stdio.h>

/* ── Flash storage for paired address ──────────────────────────────────── */
/* Store 6-byte BD_ADDR + 2-byte magic in last sector of 2 MB flash */
#define FLASH_TARGET_OFFSET   (2u * 1024u * 1024u - 4096u)
#define FLASH_MAGIC_HI        0x4A    /* 'J' */
#define FLASH_MAGIC_LO        0xAC    /* paired-address marker */

static bd_addr_t s_paired_addr;
static bool      s_has_paired_addr = false;

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

/* ── Report parsing ─────────────────────────────────────────────────────── */
static void process_hid_report(const uint8_t *report, uint16_t len)
{
    if (len < 8) return;

    uint8_t modifiers   = report[0];
    const uint8_t *keys = &report[2]; /* keycodes at bytes 2-7 */

    /* Detect key-up: in previous but not in current */
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

    /* Detect key-down: in current but not in previous */
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
    uint8_t buf[8];
    memcpy(buf, addr, 6);
    buf[6] = FLASH_MAGIC_HI;
    buf[7] = FLASH_MAGIC_LO;
    /* Disable interrupts while programming flash */
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buf, 8);
    restore_interrupts(ints);
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

/* ── BTstack packet handler ─────────────────────────────────────────────── */
static void packet_handler(uint8_t packet_type, uint16_t channel,
                            uint8_t *packet, uint16_t size)
{
    (void)channel; (void)size;

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {

                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                        printf("BT: stack working\n");
                        if (s_has_paired_addr) {
                            printf("BT: reconnecting to stored device\n");
                            hid_host_connect(s_paired_addr,
                                             HID_PROTOCOL_MODE_BOOT,
                                             &s_hid_cid);
                        } else {
                            printf("BT: starting inquiry\n");
                            gap_inquiry_start(10);
                        }
                    }
                    break;

                case GAP_EVENT_INQUIRY_RESULT: {
                    bd_addr_t addr;
                    gap_event_inquiry_result_get_bd_addr(packet, addr);
                    uint32_t cod =
                        gap_event_inquiry_result_get_class_of_device(packet);
                    /* HID keyboard COD major=Peripheral(0x05), minor=Keyboard(0x40) */
                    if ((cod & 0x1FFFu) == 0x0540u) {
                        printf("BT: found HID keyboard\n");
                        gap_inquiry_stop();
                        hid_host_connect(addr, HID_PROTOCOL_MODE_BOOT,
                                         &s_hid_cid);
                    }
                    break;
                }

                case GAP_EVENT_INQUIRY_COMPLETE:
                    /* No keyboard found; retry */
                    if (!s_hid_cid) {
                        printf("BT: inquiry complete, retrying\n");
                        gap_inquiry_start(10);
                    }
                    break;

                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)) {

                        case HID_SUBEVENT_CONNECTION_OPENED: {
                            uint8_t status =
                                hid_subevent_connection_opened_get_status(packet);
                            if (status == ERROR_CODE_SUCCESS) {
                                s_hid_cid =
                                    hid_subevent_connection_opened_get_hid_cid(packet);
                                bd_addr_t addr;
                                hid_subevent_connection_opened_get_bd_addr(packet, addr);
                                memcpy(s_paired_addr, addr, 6);
                                s_has_paired_addr = true;
                                save_paired_addr(s_paired_addr);
                                printf("BT: HID connected\n");
                            } else {
                                printf("BT: connect failed 0x%02x, rescanning\n",
                                       status);
                                s_hid_cid = 0;
                                s_has_paired_addr = false;
                                gap_inquiry_start(10);
                            }
                            break;
                        }

                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            printf("BT: disconnected, reconnecting\n");
                            s_hid_cid = 0;
                            if (s_has_paired_addr) {
                                hid_host_connect(s_paired_addr,
                                                 HID_PROTOCOL_MODE_BOOT,
                                                 &s_hid_cid);
                            } else {
                                gap_inquiry_start(10);
                            }
                            break;

                        case HID_SUBEVENT_REPORT: {
                            const uint8_t *rpt =
                                hid_subevent_report_get_report(packet);
                            uint16_t rpt_len =
                                hid_subevent_report_get_report_len(packet);
                            process_hid_report(rpt, rpt_len);
                            break;
                        }

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void bt_hid_init(bt_hid_key_down_cb_t on_key_down,
                 bt_hid_key_up_cb_t   on_key_up)
{
    s_on_key_down = on_key_down;
    s_on_key_up   = on_key_up;

    memset(s_prev_keycodes, 0, sizeof(s_prev_keycodes));

    /* Try to load previously paired BD_ADDR from flash */
    s_has_paired_addr = load_paired_addr(s_paired_addr);
    if (s_has_paired_addr) {
        printf("BT: loaded paired address from flash\n");
    }

    /* Initialise BTstack layers */
    l2cap_init();
    sdp_init();
    gap_set_default_link_policy_settings(
        LM_LINK_POLICY_ENABLE_SNIFF_MODE |
        LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    gap_set_allow_role_switch(true);

    /* HID Host */
    hid_host_init(NULL, 0);
    hid_host_register_packet_handler(packet_handler);

    /* Register for HCI events */
    s_hci_event_cb_reg.callback = &packet_handler;
    hci_add_event_handler(&s_hci_event_cb_reg);

    /* Power on */
    hci_power_control(HCI_POWER_ON);

    /* Run loop – never returns */
    btstack_run_loop_execute();
}
