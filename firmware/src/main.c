/**
 * main.c – jamin-controller firmware entry point.
 *
 * Initialises MIDI UART, application state, and Bluetooth HID host.
 * The BT init call never returns (it runs the BTstack event loop).
 */

#include "pico/stdlib.h"
#include "midi.h"
#include "keyboard.h"
#include "bt_hid.h"
#include <stdio.h>

/* Global application state */
static jamin_state_t g_state;

/* ── BT HID callbacks ───────────────────────────────────────────────────── */

static void on_key_down(uint8_t keycode, uint8_t modifiers)
{
    keyboard_key_down(&g_state, keycode, modifiers);
}

static void on_key_up(uint8_t keycode, uint8_t modifiers)
{
    keyboard_key_up(&g_state, keycode, modifiers);
}

/* ── Entry point ────────────────────────────────────────────────────────── */

int main(void)
{
    stdio_init_all();
    printf("jamin-controller starting\n");

    /* Initialise MIDI UART */
    //midi_init();
    //printf("MIDI UART ready\n");

    /* Initialise keyboard state */
    keyboard_state_init(&g_state);
    printf("Keyboard state ready (project=0, key=C, var=0)\n");

    /* Start Bluetooth HID host – never returns */
    bt_hid_init(on_key_down, on_key_up);

    /* Unreachable */
    return 0;
}
