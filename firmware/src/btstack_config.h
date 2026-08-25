#ifndef _PICO_BTSTACK_BTSTACK_CONFIG_H
#define _PICO_BTSTACK_BTSTACK_CONFIG_H

// Required configurations
#define ENABLE_CLASSIC
#define ENABLE_L2CAP_LE
#define ENABLE_HID_HOST
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL

// Restore Buffer Allocations (FIXES THE COMPILER ERROR)
#define HCI_ACL_PAYLOAD_SIZE             (1691 + 4) // Restored missing engine variable
#define HCI_OUTGOING_PRE_BUFFER_SIZE     4

// Required Memory Pool Settings
#define MAX_NR_HCI_CONNECTIONS           2
#define MAX_NR_L2CAP_CHANNELS            4
#define MAX_NR_L2CAP_SERVICES            3
#define MAX_NR_HID_HOST_CONNECTIONS      1
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2

#endif /* _PICO_BTSTACK_BTSTACK_CONFIG_H */
