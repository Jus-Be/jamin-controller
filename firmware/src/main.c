#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

typedef enum {
    BT_STATE_IDLE = 0,
    BT_STATE_ENTERING_DISCOVERY,
    BT_STATE_DISCOVERABLE,
    BT_STATE_PAIRING,
    BT_STATE_PAIRED,
    BT_STATE_FAILED,
    BT_STATE_TIMEOUT
} bt_state_t;

typedef enum {
    BT_EVENT_DISCOVERY_REQUEST = 0,
    BT_EVENT_PAIRING_STARTED,
    BT_EVENT_PAIRED,
    BT_EVENT_FAILED,
    BT_EVENT_TIMEOUT,
    BT_EVENT_DISCONNECTED,
    BT_EVENT_CANCEL
} bt_event_t;

static const uint LED_PIN = PICO_DEFAULT_LED_PIN;
static const uint BOOTSEL_CS_PIN_INDEX = 1;

static const uint32_t LOOP_SLEEP_MS = 10;
static const uint32_t BOOTSEL_DEBOUNCE_MS = 40;
static const uint32_t BOOTSEL_HOLD_TRIGGER_MS = 1200;
static const uint32_t DISCOVERY_TIMEOUT_MS = 60000;
static const uint32_t PAIRING_TIMEOUT_MS = 30000;
static const uint32_t ERROR_DISPLAY_MS = 4000;
static const uint32_t DISCOVERABLE_BLINK_MS = 500;
static const uint32_t PAIRING_BLINK_MS = 150;
static const uint32_t ERROR_BLINK_MS = 100;

static bt_state_t g_state = BT_STATE_IDLE;
static uint32_t g_state_entered_at_ms = 0;

static inline uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static bool __no_inline_not_in_flash_func(read_bootsel_button)(void) {
    uint32_t flags = save_and_disable_interrupts();
    hw_write_masked(
        &ioqspi_hw->io[BOOTSEL_CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS
    );
    for (volatile int i = 0; i < 1000; ++i) {
    }
    bool pressed = !(sio_hw->gpio_hi_in & (1u << BOOTSEL_CS_PIN_INDEX));
    hw_write_masked(
        &ioqspi_hw->io[BOOTSEL_CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS
    );
    restore_interrupts(flags);
    return pressed;
}

static void set_state(bt_state_t next_state) {
    g_state = next_state;
    g_state_entered_at_ms = now_ms();
}

static void handle_event(bt_event_t event) {
    switch (event) {
        case BT_EVENT_DISCOVERY_REQUEST:
            set_state(BT_STATE_ENTERING_DISCOVERY);
            break;
        case BT_EVENT_PAIRING_STARTED:
            if (g_state == BT_STATE_DISCOVERABLE) {
                set_state(BT_STATE_PAIRING);
            }
            break;
        case BT_EVENT_PAIRED:
            set_state(BT_STATE_PAIRED);
            break;
        case BT_EVENT_FAILED:
            set_state(BT_STATE_FAILED);
            break;
        case BT_EVENT_TIMEOUT:
            set_state(BT_STATE_TIMEOUT);
            break;
        case BT_EVENT_DISCONNECTED:
            set_state(BT_STATE_IDLE);
            break;
        case BT_EVENT_CANCEL:
            set_state(BT_STATE_IDLE);
            break;
    }
}

static void service_state_transitions(void) {
    uint32_t elapsed = now_ms() - g_state_entered_at_ms;

    switch (g_state) {
        case BT_STATE_ENTERING_DISCOVERY:
            set_state(BT_STATE_DISCOVERABLE);
            break;
        case BT_STATE_DISCOVERABLE:
            if (elapsed >= DISCOVERY_TIMEOUT_MS) {
                set_state(BT_STATE_TIMEOUT);
            }
            break;
        case BT_STATE_PAIRING:
            if (elapsed >= PAIRING_TIMEOUT_MS) {
                set_state(BT_STATE_TIMEOUT);
            }
            break;
        case BT_STATE_FAILED:
        case BT_STATE_TIMEOUT:
            if (elapsed >= ERROR_DISPLAY_MS) {
                set_state(BT_STATE_IDLE);
            }
            break;
        case BT_STATE_IDLE:
        case BT_STATE_PAIRED:
            break;
    }
}

static void update_led_pattern(void) {
    uint32_t elapsed = now_ms() - g_state_entered_at_ms;
    bool led_on = false;

    switch (g_state) {
        case BT_STATE_IDLE:
            led_on = false;
            break;
        case BT_STATE_ENTERING_DISCOVERY:
        case BT_STATE_DISCOVERABLE:
            led_on = ((elapsed / DISCOVERABLE_BLINK_MS) % 2u) == 0u;
            break;
        case BT_STATE_PAIRING:
            led_on = ((elapsed / PAIRING_BLINK_MS) % 2u) == 0u;
            break;
        case BT_STATE_PAIRED:
            led_on = true;
            break;
        case BT_STATE_FAILED:
        case BT_STATE_TIMEOUT: {
            uint32_t step = elapsed / ERROR_BLINK_MS;
            uint32_t phase = step % 8u;
            led_on = (phase < 2u) || (phase >= 4u && phase < 6u);
            break;
        }
    }

    gpio_put(LED_PIN, led_on);
}

static void service_bootsel_discovery_trigger(void) {
    static bool stable_pressed = false;
    static bool raw_last = false;
    static uint32_t raw_changed_at_ms = 0;
    static uint32_t stable_changed_at_ms = 0;
    static bool hold_handled = false;

    uint32_t t = now_ms();
    bool raw = read_bootsel_button();

    if (raw != raw_last) {
        raw_last = raw;
        raw_changed_at_ms = t;
    }

    if ((t - raw_changed_at_ms) >= BOOTSEL_DEBOUNCE_MS && raw != stable_pressed) {
        stable_pressed = raw;
        stable_changed_at_ms = t;
        hold_handled = false;
    }

    if (stable_pressed && !hold_handled && (t - stable_changed_at_ms) >= BOOTSEL_HOLD_TRIGGER_MS) {
        hold_handled = true;
        if (g_state != BT_STATE_PAIRING) {
            handle_event(BT_EVENT_DISCOVERY_REQUEST);
        }
    }

    if (!stable_pressed) {
        hold_handled = false;
    }
}

static void service_debug_serial_events(void) {
    int ch = getchar_timeout_us(0);
    if (ch == PICO_ERROR_TIMEOUT) {
        return;
    }

    switch ((char)ch) {
        case 'r':
            handle_event(BT_EVENT_DISCOVERY_REQUEST);
            break;
        case 'p':
            handle_event(BT_EVENT_PAIRING_STARTED);
            break;
        case 's':
            handle_event(BT_EVENT_PAIRED);
            break;
        case 'f':
            handle_event(BT_EVENT_FAILED);
            break;
        case 't':
            handle_event(BT_EVENT_TIMEOUT);
            break;
        case 'd':
            handle_event(BT_EVENT_DISCONNECTED);
            break;
        case 'c':
            handle_event(BT_EVENT_CANCEL);
            break;
        default:
            break;
    }
}

int main(void) {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    set_state(BT_STATE_IDLE);

    while (true) {
        service_bootsel_discovery_trigger();
        service_debug_serial_events();
        service_state_transitions();
        update_led_pattern();
        sleep_ms(LOOP_SLEEP_MS);
    }
}
