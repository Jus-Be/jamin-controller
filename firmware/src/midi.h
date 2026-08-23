/**
 * midi.h
 *
 * Thin MIDI-over-UART driver for the WAV Trigger Pro.
 *
 * The WAV Trigger Pro listens for standard MIDI messages on its UART RX pin
 * at 31250 baud.  The Pico sends Note-On / Note-Off messages where:
 *   - The MIDI channel maps to a track category (chord/bass/perc/riff/hit).
 *   - The MIDI note number encodes the WAV file index within that category.
 *   - The WAV Trigger Pro uses "polyphonic toggle" mode when looping:
 *       Note-On  → start loop / one-shot
 *       Note-Off → stop loop
 *
 * WAV Trigger Pro MIDI channel assignments:
 *   CH 1  – chord loops
 *   CH 2  – bass loops
 *   CH 3  – percussion loops
 *   CH 4  – melody riffs (one-shot)
 *   CH 10 – one-off hit sounds
 *
 * Because the WAV Trigger's MIDI note range is 0-127, WAV file indices larger
 * than 127 are split across the note and channel offset fields using a bank
 * system: we map WAV index to (bank, note) and select an unused MIDI channel
 * for the bank where needed.  For simplicity the firmware caps each category
 * so that a single note byte is sufficient (see MIDI_NOTE_MAX).
 */

#ifndef MIDI_H
#define MIDI_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdint.h>

/* UART instance wired to WAV Trigger Pro MIDI IN */
#define MIDI_UART_ID     uart1
#define MIDI_UART_TX_PIN 4    /* GPIO 4 → WAV Trigger MIDI RX via Grove/QWIIC */
#define MIDI_UART_RX_PIN 5    /* GPIO 5 (unused, but reserved) */
#define MIDI_BAUD_RATE   31250

/* MIDI channel assignments (1-based, as per MIDI spec) */
#define MIDI_CH_CHORD    1
#define MIDI_CH_BASS     2
#define MIDI_CH_PERC     3
#define MIDI_CH_RIFF     4
#define MIDI_CH_HIT      10

/* MIDI status bytes (channel embedded in low nibble, 0-based) */
#define MIDI_NOTE_OFF    0x80
#define MIDI_NOTE_ON     0x90
#define MIDI_NOTE_MAX    127

/* MIDI velocity */
#define MIDI_VEL_DEFAULT 100
#define MIDI_VEL_OFF     0

/**
 * Initialise the UART for MIDI output.
 */
void midi_init(void);

/**
 * Send a MIDI Note-On message.
 *
 * @param channel   MIDI channel (1-based, 1-16)
 * @param note      MIDI note number (0-127)
 * @param velocity  Velocity (1-127)
 */
void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Send a MIDI Note-Off message.
 *
 * @param channel  MIDI channel (1-based)
 * @param note     MIDI note number (0-127)
 */
void midi_note_off(uint8_t channel, uint8_t note);

/**
 * Trigger a WAV file by its 1-based SD-card index on the given MIDI channel.
 *
 * For the WAV Trigger Pro, note 0 maps to WAV file 1, note 1 to file 2, etc.
 * Files beyond note 127 require a bank offset applied via the channel number.
 *
 * @param wav_index  1-based WAV file index (1..4096)
 * @param channel    MIDI channel category (e.g. MIDI_CH_CHORD)
 * @param velocity   Velocity (use MIDI_VEL_OFF to stop a looping file)
 */
void midi_trigger_wav(uint16_t wav_index, uint8_t channel, uint8_t velocity);

#endif /* MIDI_H */
