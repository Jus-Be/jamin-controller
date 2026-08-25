/**
 * main.c – jamin-controller firmware entry point.
 *
 * Initialises MIDI UART, application state, and Bluetooth HID host.
 * The BT init call never returns (it runs the BTstack event loop).
 */

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/cyw43_arch.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "hci.h"
#include "bt_hid.h"
#include "midi.h"
#include "keyboard.h"
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
	
	cyw43_arch_init();
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);		

    //while (!stdio_usb_connected()) {
        //sleep_ms(10);
    //}
	
    //printf("jamin-controller starting\n");	

    /* Initialise MIDI UART */
    //midi_init();
    //printf("MIDI UART ready\n");

    /* Initialise keyboard state */
    //keyboard_state_init(&g_state);
    //printf("Keyboard state ready (project=0, key=C, var=0)\n");
	
	sleep_ms(5000);
    
	printf("Readsy for debugging\n");	
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
	
    /* Start Bluetooth HID host – never returns */
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_pico_get_instance());
    bt_hid_init(on_key_down, on_key_up);
    hci_power_control(HCI_POWER_ON); // Power on the radio
    
    // --- RUNLOOP ---
    btstack_run_loop_execute(); // Replaces while(true)
    // -------------	

    /* Unreachable */
    return 0;
}
