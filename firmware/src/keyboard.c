/**
 * keyboard.c – Keyboard zone mapping and MIDI event dispatch.
 */

#include "keyboard.h"
#include <stdbool.h>
#include "midi.h"
#include "wav_index.h"
#include <string.h>
#include <stdio.h>

/* ── Internal helpers ───────────────────────────────────────────────────── */

/* Zone 2 layout: column index (0-4) for each left-hand key.
 * Row 0 (major): Q W E R T  → HID 0x14 0x1A 0x08 0x15 0x17
 * Row 1 (minor): A S D F G  → HID 0x04 0x16 0x07 0x09 0x0A
 * Row 2 (sus)  : Z X C V B  → HID 0x1D 0x1B 0x06 0x19 0x05
 */
static const struct { uint8_t keycode; uint8_t row; uint8_t col; } zone2_map[] = {
    { HID_KEY_Q, 0, 0 }, { HID_KEY_W, 0, 1 }, { HID_KEY_E, 0, 2 },
    { HID_KEY_R, 0, 3 }, { HID_KEY_T, 0, 4 },
    { HID_KEY_A, 1, 0 }, { HID_KEY_S, 1, 1 }, { HID_KEY_D, 1, 2 },
    { HID_KEY_F, 1, 3 }, { HID_KEY_G, 1, 4 },
    { HID_KEY_Z, 2, 0 }, { HID_KEY_X, 2, 1 }, { HID_KEY_C, 2, 2 },
    { HID_KEY_V, 2, 3 }, { HID_KEY_B, 2, 4 },
};
#define ZONE2_KEY_COUNT  (sizeof(zone2_map) / sizeof(zone2_map[0]))

/* Zone 3 layout: riff slot index for each right-hand key.
 * Row 0: Y U I O P [ ] \  → slots 0-7
 * Row 1: H J K L ; '      → slots 8-13
 * Row 2: N M , . /        → slots 14-18
 */
static const struct { uint8_t keycode; uint8_t slot; } zone3_map[] = {
    { HID_KEY_Y,    0  }, { HID_KEY_U,    1  }, { HID_KEY_I,    2  },
    { HID_KEY_O,    3  }, { HID_KEY_P,    4  },
    /* [ ] \ – HID 0x2F 0x30 0x31 */
    { 0x2F,         5  }, { 0x30,         6  }, { 0x31,         7  },
    { HID_KEY_H,    8  }, { HID_KEY_J,    9  }, { HID_KEY_K,    10 },
    { HID_KEY_L,    11 }, { 0x33,         12 }, { 0x34,         13 },
    /* ; ' – HID 0x33 0x34 */
    { HID_KEY_N,    14 }, { HID_KEY_M,    15 },
    /* , . / – HID 0x36 0x37 0x38 */
    { 0x36,         16 }, { 0x37,         17 }, { 0x38,         18 },
};
#define ZONE3_KEY_COUNT  (sizeof(zone3_map) / sizeof(zone3_map[0]))

/* Diatonic scale degrees for the 5 columns (semitone offsets from root) */
static const uint8_t DIATONIC_OFFSETS[5] = { 0, 2, 4, 7, 9 }; /* I II III V VI */

/* Declared here, defined in Zone 2 section below */
static chord_type_t s_held_z2_type[ZONE2_KEY_COUNT];
static bool         s_held_z2_active[ZONE2_KEY_COUNT];

/* ── State management ───────────────────────────────────────────────────── */

void keyboard_state_init(jamin_state_t *state)
{
    memset(state, 0, sizeof(*state));
    memset(s_held_z2_type,   0, sizeof(s_held_z2_type));
    memset(s_held_z2_active, 0, sizeof(s_held_z2_active));
}

/* Recompute the harmony class from the active chord list */
static void update_harmony_class(jamin_state_t *state)
{
    uint8_t mask = 0;
    for (uint8_t i = 0; i < state->active_chord_count; i++) {
        mask |= (uint8_t)(1u << (uint8_t)state->active_chords_type[i]);
    }
    state->harmony_class = mask;
}

/* Add a chord to the active list (no duplicates) */
static void active_chord_add(jamin_state_t *state, uint8_t root, chord_type_t type)
{
    for (uint8_t i = 0; i < state->active_chord_count; i++) {
        if (state->active_chords_root[i] == root &&
            state->active_chords_type[i] == type) {
            return; /* already active */
        }
    }
    if (state->active_chord_count < MAX_ACTIVE_CHORDS) {
        uint8_t idx = state->active_chord_count++;
        state->active_chords_root[idx] = root;
        state->active_chords_type[idx] = type;
        update_harmony_class(state);
    }
}

/* Remove a chord from the active list */
static void active_chord_remove(jamin_state_t *state, uint8_t root, chord_type_t type)
{
    for (uint8_t i = 0; i < state->active_chord_count; i++) {
        if (state->active_chords_root[i] == root &&
            state->active_chords_type[i] == type) {
            /* Shift remaining entries */
            for (uint8_t j = i; j < state->active_chord_count - 1u; j++) {
                state->active_chords_root[j] = state->active_chords_root[j + 1];
                state->active_chords_type[j] = state->active_chords_type[j + 1];
            }
            state->active_chord_count--;
            update_harmony_class(state);
            return;
        }
    }
}

/* ── Zone 2: chord loop trigger ─────────────────────────────────────────── */

static bool find_zone2_key(uint8_t keycode, uint8_t *out_row, uint8_t *out_col)
{
    for (uint8_t i = 0; i < ZONE2_KEY_COUNT; i++) {
        if (zone2_map[i].keycode == keycode) {
            *out_row = zone2_map[i].row;
            *out_col = zone2_map[i].col;
            return true;
        }
    }
    return false;
}

/* Returns the slot index in zone2_map for a keycode, or 0xFF if not found */
static uint8_t zone2_slot(uint8_t keycode)
{
    for (uint8_t i = 0; i < ZONE2_KEY_COUNT; i++) {
        if (zone2_map[i].keycode == keycode) return i;
    }
    return 0xFF;
}

static void handle_zone2_down(jamin_state_t *state, uint8_t keycode,
                              uint8_t row, uint8_t col, uint8_t modifiers)
{
    /* Determine chord type from row + shift modifier */
    chord_type_t type;
    if (row == 0) {
        type = CHORD_MAJOR;
    } else if (row == 1) {
        type = CHORD_MINOR;
    } else {
        type = (modifiers & HID_MOD_LSHIFT) ? CHORD_SUS4 : CHORD_SUS2;
    }

    /* Persist resolved type so key-up can use it unchanged */
    uint8_t slot = zone2_slot(keycode);
    if (slot != 0xFF) {
        s_held_z2_type[slot]   = type;
        s_held_z2_active[slot] = true;
    }

    /* Determine chromatic root: song_key + diatonic column offset */
    uint8_t root = (uint8_t)((state->song_key + DIATONIC_OFFSETS[col]) % 12u);

    /* Derive WAV index */
    uint16_t wav = wav_index_chord(state->project, root, type, state->variation);
    if (wav == 0) return;

    /* Track active chords for harmony calculation */
    active_chord_add(state, root, type);

    /* Send MIDI Note-On to toggle/start the loop */
    midi_trigger_wav(wav, MIDI_CH_CHORD, MIDI_VEL_DEFAULT);
    printf("CHORD ON  root=%d type=%d wav=%d\n", root, (int)type, wav);
}

static void handle_zone2_up(jamin_state_t *state, uint8_t keycode,
                            uint8_t col)
{
    uint8_t slot = zone2_slot(keycode);
    if (slot == 0xFF || !s_held_z2_active[slot]) return;

    chord_type_t type = s_held_z2_type[slot];
    s_held_z2_active[slot] = false;

    uint8_t root = (uint8_t)((state->song_key + DIATONIC_OFFSETS[col]) % 12u);
    active_chord_remove(state, root, type);

    /* Note: chord loops continue playing after key-up (toggle model).
     * A second key-down on the same key will stop the loop.
     * We do NOT send Note-Off here intentionally.
     */
}

/* ── Zone 3: melody riff trigger ────────────────────────────────────────── */

static bool find_zone3_key(uint8_t keycode, uint8_t *out_slot)
{
    for (uint8_t i = 0; i < ZONE3_KEY_COUNT; i++) {
        if (zone3_map[i].keycode == keycode) {
            *out_slot = zone3_map[i].slot;
            return true;
        }
    }
    return false;
}

static void handle_zone3_down(jamin_state_t *state, uint8_t slot)
{
    /* Riff slot wraps around RIFFS_PER_KEY */
    uint8_t riff_slot = slot % RIFFS_PER_KEY;

    uint16_t wav = wav_index_riff(state->harmony_class, state->song_key, riff_slot);
    if (wav == 0) return;

    midi_trigger_wav(wav, MIDI_CH_RIFF, MIDI_VEL_DEFAULT);
    printf("RIFF  class=%d key=%d slot=%d wav=%d\n",
           state->harmony_class, state->song_key, riff_slot, wav);
}

/* ── Zone 1: global key handlers ────────────────────────────────────────── */

static void handle_global_down(jamin_state_t *state, uint8_t keycode,
                               uint8_t modifiers)
{
    /* F1-F12 → set song key */
    if (keycode >= HID_KEY_F1 && keycode <= HID_KEY_F12) {
        state->song_key = keycode - HID_KEY_F1;  /* 0=C .. 11=B */
        printf("SONG KEY → %d\n", state->song_key);
        return;
    }

    /* 1-9 → select project 0-8 */
    if (keycode >= HID_KEY_1 && keycode <= HID_KEY_9) {
        state->project = keycode - HID_KEY_1;
        printf("PROJECT → %d\n", state->project);
        return;
    }

    /* 0 → project 9 */
    if (keycode == HID_KEY_0) {
        state->project = 9;
        printf("PROJECT → 9\n");
        return;
    }

    /* - and = for projects 10-19 */
    if (keycode == HID_KEY_MINUS) {
        state->project = (state->project < MAX_PROJECTS - 1) ?
                         state->project + 1 : state->project;
        printf("PROJECT → %d\n", state->project);
        return;
    }

    /* Arrow Up/Down: change variation */
    if (keycode == HID_KEY_UP || keycode == HID_KEY_DOWN) {
        state->variation = (state->variation == 0) ? 1 : 0;
        printf("VARIATION → %d\n", state->variation);
        return;
    }

    /* SPACE: stop all loops by sending Note-Off on all active chord WAVs */
    if (keycode == HID_KEY_SPACE) {
        for (uint8_t i = 0; i < state->active_chord_count; i++) {
            uint16_t wav = wav_index_chord(state->project,
                                           state->active_chords_root[i],
                                           state->active_chords_type[i],
                                           state->variation);
            if (wav) midi_trigger_wav(wav, MIDI_CH_CHORD, MIDI_VEL_OFF);
        }
        /* Stop bass and perc too with a simple all-notes-off CC if needed */
        /* WAV Trigger supports this via MIDI CC 123 ch 0 */
        uint8_t msg[3] = { 0xB0, 123, 0 }; /* All Notes Off CH1 */
        /* send on all category channels */
        for (uint8_t ch = 1; ch <= 10; ch++) {
            msg[0] = (uint8_t)(0xB0u | ((ch - 1u) & 0x0Fu));
            uart_write_blocking(MIDI_UART_ID, msg, 3);
        }
        memset(state->active_chords_root, 0, sizeof(state->active_chords_root));
        memset(state->active_chords_type, 0, sizeof(state->active_chords_type));
        state->active_chord_count = 0;
        state->harmony_class = 0;
        printf("STOP ALL\n");
        return;
    }

    /* ENTER: trigger/toggle percussion loop */
    if (keycode == HID_KEY_ENTER) {
        uint16_t wav = wav_index_perc(state->project,
                                      (perc_slot_t)(PERC_VAR1 + state->variation));
        if (wav) midi_trigger_wav(wav, MIDI_CH_PERC, MIDI_VEL_DEFAULT);
        printf("PERC ON  wav=%d\n", wav);
        return;
    }

    /* BACKSPACE: stop current riff using a single MIDI All Notes Off (CC 123) */
    if (keycode == HID_KEY_BACKSPACE) {
        uint8_t msg[3] = {
            (uint8_t)(0xB0u | ((MIDI_CH_RIFF - 1u) & 0x0Fu)),
            123u, 0u
        };
        uart_write_blocking(MIDI_UART_ID, msg, 3);
        printf("RIFF STOP\n");
        return;
    }

    (void)modifiers;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void keyboard_key_down(jamin_state_t *state, uint8_t keycode, uint8_t modifiers)
{
    uint8_t row, col, slot;

    /* Check Zone 2 first */
    if (find_zone2_key(keycode, &row, &col)) {
        handle_zone2_down(state, keycode, row, col, modifiers);
        return;
    }

    /* Check Zone 3 */
    if (find_zone3_key(keycode, &slot)) {
        handle_zone3_down(state, slot);
        return;
    }

    /* Zone 1 – global */
    handle_global_down(state, keycode, modifiers);
}

void keyboard_key_up(jamin_state_t *state, uint8_t keycode, uint8_t modifiers)
{
    uint8_t row, col;
    (void)modifiers; /* not needed; type was stored at key-down */
    if (find_zone2_key(keycode, &row, &col)) {
        handle_zone2_up(state, keycode, col);
    }
    /* Zone 3 and Zone 1 keys have no key-up action */
}
