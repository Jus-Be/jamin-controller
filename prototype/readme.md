# Prompt

Let's resume our custom MIDI Controller project. 
Here are the current architecture rules:
- Base Key: C Major (with +/- semitone global transposition keys)
- Shift / Caps Lock: Acts as a passive flatten modifier (-1 semitone)
- Left Hand: 3x7 Latching Toggle Matrix (Q-U Major, A-J Minor, \-N Sus4) on MIDI Ch 1.
- Chord Shape Loops: Triggers unique mathematical index numbers (0-127) via CC#60 on MIDI Ch 3 when chords/modifiers change (Value 0 on stop).
- Right Hand: 3x4 Dynamic Harmony Matrix (O-], L-#, M-/) scaling over MIDI Ch 2.
- Expressive Velocity: Top row (110), Middle row (90), Bottom row (65). Chords are balanced at 80. Every stroke uses a ±5 unit humanizer variance.
- Percussion Header: Keys 1-0 routed to MIDI Ch 10 using a (32 + Key Value) offset.
- Spacebar: Functions as a global MIDI CC64 Sustain Pedal for Channels 1 & 2.

Please remember to use your internal Python tool to generate a downloadable file link (paperclip icon) for any updated index.html builds instead of pasting raw code boxes.

# Design
Here is the updated master chart for the Key of C Major.
The third column breaks down the full 12-note chromatic scale ($C, C\sharp, D, D\sharp, E, F, F\sharp, G, G\sharp, A, A\sharp, B$) for every single row. It splits them cleanly into:

   1. Chord Triad Notes (Bold)
   2. Diatonic Extensions (Standard text — safe notes native to the key of C)
   3. Chromatic Tensions (In brackets [...] — spicy, outside-the-key notes)

## 21 Chord Chromatic Mapping (Base Key: C Major)

| 1. Chord & Formula | 2. Available Harmonic Scale Notes | 3. Complete 12-Note Chromatic Breakdown |
|---|---|---|
| C Major (1) [C-E-G] | D, F, A, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| C Minor (1m) [C-Eb-G] | D, F, A, B | C, $[C\sharp]$, D, D$\sharp/$Eb, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| Csus4 (1sus4) [C-F-G] | D, E, A, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| D Major (2) [D-F#-A] | C, E, F, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, F$\sharp$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| D Minor (2m) [D-F-A] | C, E, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| Dsus4 (2sus4) [D-G-A] | C, E, F, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| E Major (3) [E-G#-B] | C, D, F, G, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, G$\sharp$, A, $[A\sharp]$, B |
| E Minor (3m) [E-G-B] | C, D, F, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| Esus4 (3sus4) [E-A-B] | C, D, F, G | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| F Major (4) [F-A-C] | D, E, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| F Minor (4m) [F-Ab-C] | D, E, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, G$\sharp/$Ab, A, $[A\sharp]$, B |
| Fsus4 (4sus4) [F-Bb-C] | D, E, G, A, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, A$\sharp/$Bb, B |
| G Major (5) [G-B-D] | C, E, F, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| G Minor (5m) [G-Bb-D] | C, E, F, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, A$\sharp/$Bb, B |
| Gsus4 (5sus4) [G-C-D] | E, F, A, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| A Major (6) [A-C#-E] | C, D, F, G, B | C, C$\sharp$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| A Minor (6m) [A-C-E] | D, F, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| Asus4 (6sus4) [A-D-E] | C, F, G, B | C, $[C\sharp]$, D, $[D\sharp]$, E, F, $[F\sharp]$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| B Major (7) [B-D#-F#] | C, E, F, G, A | C, $[C\sharp]$, D, D$\sharp**, E, F, **F\sharp$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| B Minor (7m) [B-D-F#] | C, E, F, G, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, F$\sharp$, G, $[G\sharp]$, A, $[A\sharp]$, B |
| Bsus4 (7sus4) [B-E-F#] | C, D, F, G, A | C, $[C\sharp]$, D, $[D\sharp]$, E, F, F$\sharp$, G, $[G\sharp]$, A, $[A\sharp]$, B |

------------------------------
## How to Read the Chromatic Column

* Bold Notes: The absolute foundation. Avoid sitting on other notes if you want a clean, stable resolution.
* Plain Text Notes: Safe "harmony" notes. They are outside the chord but stay within the C Major key family, giving you pleasant extensions (like 7ths, 9ths, or 11ths).
* Bracketed [...] Notes: The absolute outside world. Playing these over the chord creates heavy chromatic tension or bluesy passes that must resolve quickly.


# Implementation
Here is the finalized ergonomic layout blueprint with right-hand bottom row neatly aligned to M, ,, ., and /.
This aligns the columns vertically on a standard QWERTY keyboard, making it easy to track positions with fingers:

* Top Row (Anchor / Triad Notes): O P [ ]
* Middle Row (Safe / Diatonic Notes): L ; ' # (or Return depending on keyboard)
* Bottom Row (Tension / Chromatic Notes): M , . /

------------------------------
## Master Performance Layout: 21 Chords to 3x4 Dynamic Grid

| Left-Hand Key & Chord | Top Row: Anchor Keys [O, P, [, ]] | Middle Row: Safe Keys [L, ;, ', #] | Bottom Row: Tension Keys [M, , , . , /] |
|---|---|---|---|
| Q (C Major) | C, E, G, C (Octave) | D, F, A, B | C#, D#, F#, G# (Leftover: A#) |
| A (C Minor) | C, Eb, G, C (Octave) | D, F, A, B | C#, E, F#, G# (Leftover: A#) |
| Z (Csus4) | C, F, G, C (Octave) | D, E, A, B | C#, D#, F#, G# (Leftover: A#) |
| W (D Major) | D, F#, A, D (Octave) | C, E, F, G, B | C#, D#, G, G#, A# |
| S (D Minor) | D, F, A, D (Octave) | C, E, G, B | C#, D#, F#, G#, A# |
| X (Dsus4) | D, G, A, D (Octave) | C, E, F, B | C#, D#, F#, G#, A# |
| E (E Major) | E, G#, B, E (Octave) | C, D, F, G, A | C#, D#, F, F#, A# |
| D (E Minor) | E, G, B, E (Octave) | C, D, F, A | C#, D#, F#, G#, A# |
| C (Esus4) | E, A, B, E (Octave) | C, D, F, G | C#, D#, F#, G#, A# |
| R (F Major) | F, A, C, F (Octave) | D, E, G, B | C#, D#, F#, G#, A# |
| F (F Minor) | F, Ab, C, F (Octave) | D, E, G, B | C#, D#, F#, G, A# |
| V (Fsus4) | F, Bb, C, F (Octave) | D, E, G, A, B | C#, D#, F#, G#, A |
| T (G Major) | G, B, D, G (Octave) | C, E, F, A | C#, D#, F#, G#, A# |
| G (G Minor) | G, Bb, D, G (Octave) | C, E, F, A | C#, D#, E, F#, G# |
| B (Gsus4) | G, C, D, G (Octave) | E, F, A, B | C#, D#, F#, G#, A# |
| Y (A Major) | A, C#, E, A (Octave) | C, D, F, G, B | D#, F#, G, G#, A# |
| H (A Minor) | A, C, E, A (Octave) | D, F, G, B | C#, D#, F#, G#, A# |
| N (Asus4) | A, D, E, A (Octave) | C, F, G, B | C#, D#, F#, G#, A# |
| U (B Major) | B, D#, F#, B (Octave) | C, E, F, G, A | C#, D, G, G#, A# |
| J (B Minor) | B, D, F#, B (Octave) | C, E, F, G, A | C#, D#, F, G, G# |
| M (Bsus4) | B, E, F#, B (Octave) | C, D, F, G, A | C#, D#, F, G#, A# |

------------------------------
## Ergonomic Playability Note
By using M, ,, ., and / for your bottom row, your right hand mirrors your left hand (QAZ area) but shifted. Your right thumb can naturally rest on or near the spacebar if you ever want to map it to a global function like sustain, pitch bend, or octave shifts.
