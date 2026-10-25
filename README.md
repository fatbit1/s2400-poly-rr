# s2400-poly-rr

An Arduino MIDI processor that lets you play an **ISLA Instruments S2400** polyphonically from a MIDI keyboard. Incoming notes are spread round-robin across several MIDI channels, one per S2400 track, so a new note doesn't cut off the one before it.

```
Keyboard (ch 3) ──► Arduino + MIDI shield ──► S2400 (tracks on ch 1–8)
```

## How it works

- Notes on the **input channel** (default 3) are assigned to the next free voice. Each voice is one MIDI channel (default 1–8).
- When all voices are busy, the **oldest note is stolen**: it gets a Note Off before its voice is reused.
- Each Note Off goes to the voice actually playing that note. Retriggering a held key and stealing voices both leave no stuck notes.
- **Everything else is passed through once and unchanged:** other channels, CCs and pitch bend on the input channel, clock, start/stop, song position and SysEx.

### Pitch modes

Set `PITCH_VIA_CC` at the top of the sketch:

| `PITCH_VIA_CC` | What each voice receives | S2400 setup |
|---|---|---|
| `1` (default) | `CC 80 = note number`, then Note On for a fixed note `60` | Load the same sample on each track and map CC 80 to pitch |
| `0` | The original note number | Tracks respond to note numbers chromatically |

## Configuration

All settings are `#define`s at the top of `s2400-poly-rr.ino`:

| Setting | Default | Meaning |
|---|---|---|
| `INPUT_CHANNEL` | 3 | Channel your keyboard sends on |
| `FIRST_OUT_CH` | 1 | First S2400 track channel |
| `NUM_VOICES` | 8 | Number of voices/channels used |
| `PITCH_VIA_CC` | 1 | Pitch mode (see above) |
| `PITCH_CC` | 80 | CC that carries the note number |
| `TRIGGER_NOTE` | 60 | Fixed note used to trigger each track |

## Hardware

- Arduino UNO (or compatible)
- MIDI shield with IN and OUT
- 2 MIDI cables
- USB cable, plus a 5 V USB charger or 9 V PSU

## Installing

1. In the Arduino IDE, install **MIDI Library** by Francois Best (FortySevenEffects) from the Library Manager.
2. Open `s2400-poly-rr.ino`.
3. Switch the MIDI shield's RX/serial switch to **OFF**. The shield shares the UNO's serial port, and uploading fails otherwise.
4. Select the board and port, then upload.
5. Switch the RX switch back **ON**, unplug USB and power the Arduino from the charger or PSU.
6. Connect keyboard MIDI OUT → shield MIDI IN, and shield MIDI OUT → S2400 MIDI IN.
