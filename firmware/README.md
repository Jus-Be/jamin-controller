# Pico firmware

This firmware adds a Bluetooth pairing/discovery state machine with LED status patterns and a BOOTSEL-triggered discovery mode.

## State model

- `IDLE`: LED off.
- `ENTERING_DISCOVERY`: transient state before discoverable.
- `DISCOVERABLE`: slow blink.
- `PAIRING`: fast blink.
- `PAIRED`: steady on.
- `FAILED`: error blink cadence, then auto-return to idle.
- `TIMEOUT`: error blink cadence, then auto-return to idle.

## BOOTSEL behavior

- Press and hold BOOTSEL for `1200ms` to request discovery mode.
- Button input is debounced (`40ms`) to avoid accidental triggers.
- While pairing is in progress, discovery re-trigger is ignored.

## Build

1. Install and configure `pico-sdk`.
2. Set `PICO_SDK_PATH`.
3. Build:

```bash
cd /home/runner/work/jamin-controller/jamin-controller/firmware
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Hardware validation checklist

- Power-on: verify LED stays off in idle.
- Hold BOOTSEL: verify transition to slow blink (discoverable).
- Start pairing (from Bluetooth stack event): verify fast blink.
- Pair success event: verify steady-on LED.
- Pair failure or timeout: verify error blink cadence, then LED off.
- Disconnect event: verify return to idle (LED off).
- Re-enter discovery with BOOTSEL after failure/disconnect.

## Optional serial debug events

If USB stdio is connected, send:

- `r` -> request discovery
- `p` -> pairing started
- `s` -> paired
- `f` -> failed
- `t` -> timeout
- `d` -> disconnected
- `c` -> cancel to idle
