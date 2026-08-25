#ifndef _PICO_BTSTACK_BTSTACK_CONFIG_H
#define _PICO_BTSTACK_BTSTACK_CONFIG_H

// ── Optimized Bluetooth Classic & HID Host Config ────────────────────────
#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR
#define ENABLE_PRINTF_HEXDUMP

#define ENABLE_CLASSIC                  // Explicitly enable for HID
#define ENABLE_L2CAP_LE                 // Required for proper stack initialization
#define ENABLE_HID_HOST                 // Enables HID subsystem
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL

// ── Resource Management ────────────────────────────────────────────────
#define MAX_NR_HCI_CONNECTIONS 2
#define MAX_NR_L2CAP_CHANNELS  4
#define MAX_NR_HID_HOST_CONNECTIONS 1
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2

// ── Hardware Setup ─────────────────────────────────────────────────────
#define HAVE_EMBEDDED_TIME_MS
#define HAVE_ASSERT
#define ENABLE_SOFTWARE_AES128

#endif /* _PICO_BTSTACK_BTSTACK_CONFIG_H */
