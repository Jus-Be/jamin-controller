/**
 * wav_index.h
 *
 * WAV file index mapping for the jamin-controller.
 *
 * SD card layout (4096 files total, 1-based index):
 *
 * ── Project section (indices 1 – 3120, 20 projects × 156 files)
 *    Each project block = 156 files:
 *      1-96   : 48 chord loops × 2 variations  (major/minor/sus2/sus4 for 12 chromatic steps)
 *      97-144 : 24 bass loops  × 2 variations  (major+minor for 12 steps)
 *      145-155: 11 percussion loops (intro, end, var1-4, fill1-4, break1-2)
 *      156    : (reserved / padding)
 *
 * ── Global section (indices 3121 – 4096)
 *    3121-3632: 512 one-off hit sounds
 *    3633-4096: 464 harmonic melody riffs organised by harmony class & key
 *               (≈ 976 riff slots when combined with project riff space;
 *                see RIFF_BASE / RIFF_TOTAL below)
 *
 * WAV Trigger Pro MIDI mapping:
 *   Each WAV file index maps directly to a MIDI note on a specific channel.
 *   Channel 1 is used for chord loops (polyphonic toggle via note-on/note-off).
 *   Channel 2 is used for bass loops.
 *   Channel 3 is used for percussion loops.
 *   Channel 10 is used for one-off hits (General MIDI drums convention).
 *   Channel 4 is used for melody riffs.
 */

#ifndef WAV_INDEX_H
#define WAV_INDEX_H

#include <stdint.h>

/* ── Limits ─────────────────────────────────────────────────────────────── */
#define MAX_PROJECTS       20
#define FILES_PER_PROJECT  156
#define CHORD_LOOPS        48   /* 12 roots × 4 types */
#define CHORD_VARIATIONS    2
#define BASS_LOOPS         24   /* 12 roots × 2 types (major/minor) */
#define BASS_VARIATIONS     2
#define PERC_LOOPS         11

/* ── Project section offsets (1-based) ──────────────────────────────────── */
#define PROJECT_BASE        1
#define CHORD_OFFSET        0                              /* +0 */
#define BASS_OFFSET         (CHORD_LOOPS * CHORD_VARIATIONS) /* +96 */
#define PERC_OFFSET         (BASS_OFFSET + BASS_LOOPS * BASS_VARIATIONS) /* +144 */

/* ── Global section ─────────────────────────────────────────────────────── */
#define GLOBAL_BASE         (MAX_PROJECTS * FILES_PER_PROJECT + 1) /* 3121 */
#define HIT_BASE            GLOBAL_BASE                 /* 3121 */
#define HIT_TOTAL           512
#define RIFF_BASE           (HIT_BASE + HIT_TOTAL)      /* 3633 */
#define RIFF_TOTAL          464

/* ── Chord types ────────────────────────────────────────────────────────── */
typedef enum {
    CHORD_MAJOR = 0,
    CHORD_MINOR = 1,
    CHORD_SUS2  = 2,
    CHORD_SUS4  = 3,
    CHORD_TYPE_COUNT
} chord_type_t;

/* ── Percussion slot indices (0-based within percussion section) ─────────── */
typedef enum {
    PERC_INTRO   = 0,
    PERC_END     = 1,
    PERC_VAR1    = 2,
    PERC_VAR2    = 3,
    PERC_VAR3    = 4,
    PERC_VAR4    = 5,
    PERC_FILL1   = 6,
    PERC_FILL2   = 7,
    PERC_FILL3   = 8,
    PERC_FILL4   = 9,
    PERC_BREAK1  = 10,
    PERC_BREAK2  = 11  /* only 11 used; slot 11 spare */
} perc_slot_t;

/* ── Harmony classes for melody riffs ───────────────────────────────────── */
/*
 * A harmony class encodes the combined chord quality of all simultaneously
 * active left-hand chord loops.  We use a bitmask over CHORD_TYPE_COUNT
 * quality bits.  In practice only a handful of combinations are musically
 * meaningful, but the full 4-bit space (16 classes) is kept for extensibility.
 */
#define HARMONY_CLASS_COUNT  16   /* 2^CHORD_TYPE_COUNT */
#define RIFFS_PER_CLASS      (RIFF_TOTAL / HARMONY_CLASS_COUNT)  /* 29 per class */
#define KEYS_PER_CLASS       12   /* chromatic steps */
#define RIFFS_PER_KEY        2    /* riff variants per key */

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * Return the 1-based SD-card index of a chord loop WAV file.
 *
 * @param project    Project number (0-based, 0..MAX_PROJECTS-1)
 * @param root       Chromatic root note (0=C .. 11=B)
 * @param type       Chord quality
 * @param variation  Loop variation (0 or 1)
 * @return           1-based WAV index, or 0 on error
 */
uint16_t wav_index_chord(uint8_t project, uint8_t root,
                         chord_type_t type, uint8_t variation);

/**
 * Return the 1-based SD-card index of a bass loop WAV file.
 *
 * @param project    Project number (0-based)
 * @param root       Chromatic root note (0..11)
 * @param minor      0 = major bass, 1 = minor bass
 * @param variation  Loop variation (0 or 1)
 * @return           1-based WAV index, or 0 on error
 */
uint16_t wav_index_bass(uint8_t project, uint8_t root,
                        uint8_t minor, uint8_t variation);

/**
 * Return the 1-based SD-card index of a percussion loop WAV file.
 *
 * @param project  Project number (0-based)
 * @param slot     Percussion slot (perc_slot_t)
 * @return         1-based WAV index, or 0 on error
 */
uint16_t wav_index_perc(uint8_t project, perc_slot_t slot);

/**
 * Return the 1-based SD-card index of a melody riff WAV file.
 *
 * @param harmony_class  Combined chord quality bitmask (0..HARMONY_CLASS_COUNT-1)
 * @param key            Song key / root (0..11)
 * @param riff_slot      Riff variant within the key (0..RIFFS_PER_KEY-1)
 * @return               1-based WAV index, or 0 on error
 */
uint16_t wav_index_riff(uint8_t harmony_class, uint8_t key, uint8_t riff_slot);

/**
 * Return the 1-based SD-card index of a one-off hit WAV file.
 *
 * @param hit_slot  Hit number (0..HIT_TOTAL-1)
 * @return          1-based WAV index, or 0 on error
 */
uint16_t wav_index_hit(uint16_t hit_slot);

#endif /* WAV_INDEX_H */
