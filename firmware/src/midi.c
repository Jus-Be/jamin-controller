/**
 * midi.c – MIDI-over-UART driver for WAV Trigger Pro.
 */

#include "midi.h"
#include <stdint.h>

void midi_init(void)
{
    uart_init(MIDI_UART_ID, MIDI_BAUD_RATE);
    gpio_set_function(MIDI_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MIDI_UART_RX_PIN, GPIO_FUNC_UART);
}

void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (channel < 1 || channel > 16) return;
    if (note > MIDI_NOTE_MAX)        return;
    uint8_t msg[3] = {
        (uint8_t)(MIDI_NOTE_ON | ((channel - 1u) & 0x0Fu)),
        note & 0x7Fu,
        velocity & 0x7Fu
    };
    uart_write_blocking(MIDI_UART_ID, msg, 3);
}

void midi_note_off(uint8_t channel, uint8_t note)
{
    if (channel < 1 || channel > 16) return;
    if (note > MIDI_NOTE_MAX)        return;
    uint8_t msg[3] = {
        (uint8_t)(MIDI_NOTE_OFF | ((channel - 1u) & 0x0Fu)),
        note & 0x7Fu,
        0x00u
    };
    uart_write_blocking(MIDI_UART_ID, msg, 3);
}

void midi_trigger_wav(uint16_t wav_index, uint8_t channel, uint8_t velocity)
{
    if (wav_index == 0) return;   /* invalid index */

    /* Convert 1-based WAV index to 0-based note */
    uint16_t zero_idx = wav_index - 1u;

    /* Bank: every 128 notes we step to the next MIDI channel bank.
     * The WAV Trigger Pro supports up to 4096 files; channels 1-16 × 128 = 2048,
     * so we use channel pairs for large indices: base_channel + bank. */
    uint8_t bank = (uint8_t)(zero_idx / 128u);
    uint8_t note = (uint8_t)(zero_idx % 128u);
    uint8_t eff_channel = channel + bank;

    if (eff_channel > 16) return; /* out of range – programming error */

    if (velocity == MIDI_VEL_OFF) {
        midi_note_off(eff_channel, note);
    } else {
        midi_note_on(eff_channel, note, velocity);
    }
}
