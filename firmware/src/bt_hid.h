/**
 * bt_hid.h
 *
 * Bluetooth Classic HID keyboard auto-pair and auto-connect for the
 * NOOX Bluetooth QWERTY keyboard (BR/EDR profile).
 *
 * The implementation uses BTstack (bundled with the Pico SDK) operating in
 * Classic BT HID Host mode.  On first boot the Pico enters inquiry/scan mode
 * and pairs with the first HID keyboard it finds.  The paired device address
 * is stored in flash so subsequent boots reconnect automatically.
 *
 * Key events are delivered via a callback registered with bt_hid_init().
 */

#ifndef BT_HID_H
#define BT_HID_H

#include <stdint.h>

/* ── Callback type ──────────────────────────────────────────────────────── */

/**
 * Called when a HID key-down event is received.
 *
 * @param keycode    USB HID keycode
 * @param modifiers  HID modifier byte (LSHIFT, LCTRL, …)
 */
typedef void (*bt_hid_key_down_cb_t)(uint8_t keycode, uint8_t modifiers);

/**
 * Called when a HID key-up event is received.
 *
 * @param keycode    USB HID keycode
 * @param modifiers  HID modifier byte
 */
typedef void (*bt_hid_key_up_cb_t)(uint8_t keycode, uint8_t modifiers);

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * Initialise BTstack and start Bluetooth Classic HID Host.
 *
 * Registers the key-down and key-up callbacks, then calls
 * btstack_run_loop_execute() which never returns.
 *
 * @param on_key_down  Callback for key-press events (must not be NULL)
 * @param on_key_up    Callback for key-release events (must not be NULL)
 */
void bt_hid_init(bt_hid_key_down_cb_t on_key_down,
                 bt_hid_key_up_cb_t   on_key_up);

#endif /* BT_HID_H */
