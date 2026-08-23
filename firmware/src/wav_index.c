/**
 * wav_index.c – WAV file index computation for jamin-controller.
 */

#include "wav_index.h"

uint16_t wav_index_chord(uint8_t project, uint8_t root,
                         chord_type_t type, uint8_t variation)
{
    if (project >= MAX_PROJECTS || root >= 12 ||
        type >= CHORD_TYPE_COUNT || variation >= CHORD_VARIATIONS) {
        return 0;
    }
    uint16_t project_base = PROJECT_BASE + (uint16_t)project * FILES_PER_PROJECT;
    /* chord index: root * 4 types + type, then * 2 variations + variation */
    uint16_t chord_idx = ((uint16_t)root * CHORD_TYPE_COUNT + (uint16_t)type)
                         * CHORD_VARIATIONS + variation;
    return project_base + CHORD_OFFSET + chord_idx;
}

uint16_t wav_index_bass(uint8_t project, uint8_t root,
                        uint8_t minor, uint8_t variation)
{
    if (project >= MAX_PROJECTS || root >= 12 ||
        minor > 1 || variation >= BASS_VARIATIONS) {
        return 0;
    }
    uint16_t project_base = PROJECT_BASE + (uint16_t)project * FILES_PER_PROJECT;
    uint16_t bass_idx = ((uint16_t)root * 2 + minor) * BASS_VARIATIONS + variation;
    return project_base + BASS_OFFSET + bass_idx;
}

uint16_t wav_index_perc(uint8_t project, perc_slot_t slot)
{
    if (project >= MAX_PROJECTS || (uint8_t)slot >= PERC_LOOPS) {
        return 0;
    }
    uint16_t project_base = PROJECT_BASE + (uint16_t)project * FILES_PER_PROJECT;
    return project_base + PERC_OFFSET + (uint16_t)slot;
}

uint16_t wav_index_riff(uint8_t harmony_class, uint8_t key, uint8_t riff_slot)
{
    if (harmony_class >= HARMONY_CLASS_COUNT ||
        key >= KEYS_PER_CLASS ||
        riff_slot >= RIFFS_PER_KEY) {
        return 0;
    }
    /* Layout: [class][key][riff_slot] */
    uint16_t idx = (uint16_t)harmony_class * (KEYS_PER_CLASS * RIFFS_PER_KEY)
                 + (uint16_t)key * RIFFS_PER_KEY
                 + riff_slot;
    if (idx >= RIFF_TOTAL) {
        return 0;
    }
    return RIFF_BASE + idx;
}

uint16_t wav_index_hit(uint16_t hit_slot)
{
    if (hit_slot >= HIT_TOTAL) {
        return 0;
    }
    return HIT_BASE + hit_slot;
}
