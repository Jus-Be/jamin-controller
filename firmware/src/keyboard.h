/**
 * keyboard.h
 *
 * Keyboard zone mapping and event handling for jamin-controller.
 *
 * The NOOX QWERTY HID keyboard is divided into three zones:
 *
 * ┌───────────────────────────────────────────────────────────────────┐
 * │  ZONE 1 – Global actions, settings, modifiers and triggers        │
 * │    ESC, F1-F12, TAB, CAPS, LSHIFT, LCTRL, LALT, RALT, RSHIFT     │
 * │    RCTRL, Insert, Delete, Home, End, PgUp, PgDn, arrow keys       │
 * ├──────────────────────────────────┬────────────────────────────────┤
 * │  ZONE 2 – Left hand chord loops  │  ZONE 3 – Right hand riffs     │
 * │    Q W E R T (row 1)             │    Y U I O P [ ] \  (row 1)    │
 * │    A S D F G (row 2)             │    H J K L ; '      (row 2)    │
 * │    Z X C V B (row 3)             │    N M , . /        (row 3)    │
 * └──────────────────────────────────┴────────────────────────────────┘
 *
 * Zone 2 keys select chord loops.  The 5 columns map to the 5 diatonic
 * chord degrees most commonly used.  The 3 rows map to chord type modifier:
 *   Row 1 (Q-T) : MAJOR chords
 *   Row 2 (A-G) : MINOR chords
 *   Row 3 (Z-B) : SUS2 / SUS4 (alternates based on LSHIFT held)
 *
 * The active song key selects the chromatic root offset.  Song key is changed
 * with the F1-F12 keys (C, C#, D, D#, E, F, F#, G, G#, A, A#, B).
 *
 * Zone 3 keys trigger melody riffs that harmonise with the active left-hand
 * chords.  Each zone-3 key maps to a riff slot within the current harmony class.
 *
 * Global keys:
 *   F1-F12  : set song key (C=F1 .. B=F12)
 *   1-9, 0  : select project 1-10
 *   -  =    : select project 11-20 (with LSHIFT for 11-12 … handled simply)
 *   Arrows  : select loop variation (Up/Down) or scroll project (Left/Right)
 *   SPACE   : stop all loops
 *   ENTER   : toggle percussion loop (current variation)
 *   BACKSP  : stop current melody riff
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "wav_index.h"

/* Maximum simultaneous active chord loops (left hand) */
#define MAX_ACTIVE_CHORDS  8

/* ── Application state ──────────────────────────────────────────────────── */
typedef struct {
    uint8_t       project;         /* 0-based project index */
    uint8_t       song_key;        /* 0=C .. 11=B */
    uint8_t       variation;       /* 0 or 1 */
    chord_type_t  active_chords_type[MAX_ACTIVE_CHORDS];
    uint8_t       active_chords_root[MAX_ACTIVE_CHORDS];
    uint8_t       active_chord_count;
    uint8_t       harmony_class;   /* bitmask of active chord types */
} jamin_state_t;

/**
 * Initialise application state to defaults.
 */
void keyboard_state_init(jamin_state_t *state);

/**
 * Process a raw HID key-down event.
 *
 * @param state    Pointer to current application state (may be modified)
 * @param keycode  USB HID keycode (see usb_hid_keys.h / HID spec)
 * @param modifiers  HID modifier byte (LSHIFT, LCTRL, etc.)
 */
void keyboard_key_down(jamin_state_t *state, uint8_t keycode, uint8_t modifiers);

/**
 * Process a raw HID key-up event.
 *
 * @param state    Pointer to current application state (may be modified)
 * @param keycode  USB HID keycode
 * @param modifiers  HID modifier byte
 */
void keyboard_key_up(jamin_state_t *state, uint8_t keycode, uint8_t modifiers);

/* ── HID modifier bit masks ─────────────────────────────────────────────── */
#define HID_MOD_LCTRL   0x01
#define HID_MOD_LSHIFT  0x02
#define HID_MOD_LALT    0x04
#define HID_MOD_LGUI    0x08
#define HID_MOD_RCTRL   0x10
#define HID_MOD_RSHIFT  0x20
#define HID_MOD_RALT    0x40
#define HID_MOD_RGUI    0x80

/* ── USB HID keycodes used (subset) ─────────────────────────────────────── */
#define HID_KEY_A        0x04
#define HID_KEY_B        0x05
#define HID_KEY_C        0x06
#define HID_KEY_D        0x07
#define HID_KEY_E        0x08
#define HID_KEY_F        0x09
#define HID_KEY_G        0x0A
#define HID_KEY_H        0x0B
#define HID_KEY_I        0x0C
#define HID_KEY_J        0x0D
#define HID_KEY_K        0x0E
#define HID_KEY_L        0x0F
#define HID_KEY_M        0x10
#define HID_KEY_N        0x11
#define HID_KEY_O        0x12
#define HID_KEY_P        0x13
#define HID_KEY_Q        0x14
#define HID_KEY_R        0x15
#define HID_KEY_S        0x16
#define HID_KEY_T        0x17
#define HID_KEY_U        0x18
#define HID_KEY_V        0x19
#define HID_KEY_W        0x1A
#define HID_KEY_X        0x1B
#define HID_KEY_Y        0x1C
#define HID_KEY_Z        0x1D
#define HID_KEY_1        0x1E
#define HID_KEY_2        0x1F
#define HID_KEY_3        0x20
#define HID_KEY_4        0x21
#define HID_KEY_5        0x22
#define HID_KEY_6        0x23
#define HID_KEY_7        0x24
#define HID_KEY_8        0x25
#define HID_KEY_9        0x26
#define HID_KEY_0        0x27
#define HID_KEY_ENTER    0x28
#define HID_KEY_ESCAPE   0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_SPACE    0x2C
#define HID_KEY_MINUS    0x2D
#define HID_KEY_EQUAL    0x2E
#define HID_KEY_F1       0x3A
#define HID_KEY_F2       0x3B
#define HID_KEY_F3       0x3C
#define HID_KEY_F4       0x3D
#define HID_KEY_F5       0x3E
#define HID_KEY_F6       0x3F
#define HID_KEY_F7       0x40
#define HID_KEY_F8       0x41
#define HID_KEY_F9       0x42
#define HID_KEY_F10      0x43
#define HID_KEY_F11      0x44
#define HID_KEY_F12      0x45
#define HID_KEY_RIGHT    0x4F
#define HID_KEY_LEFT     0x50
#define HID_KEY_DOWN     0x51
#define HID_KEY_UP       0x52

#endif /* KEYBOARD_H */
