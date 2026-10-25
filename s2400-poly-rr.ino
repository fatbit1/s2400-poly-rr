/*
  s2400-poly-rr — play an ISLA S2400 polyphonically with round-robin track triggering

  Notes arriving on INPUT_CHANNEL are spread across NUM_VOICES MIDI channels
  (one per S2400 track), so several notes can sound at the same time.
  Everything else (clock, transport, SysEx, other channels) is passed through.

  Two pitch modes (see PITCH_VIA_CC below):
    1 = each voice gets a CC (PITCH_CC, default 80) carrying the played note
        number, followed by a fixed TRIGGER_NOTE. Map the CC to pitch on the
        S2400 tracks.
    0 = the original note number is sent unchanged on the voice channel.

  Hardware: Arduino UNO + MIDI shield (IN and OUT).
  Library:  "MIDI Library" by Francois Best / FortySevenEffects (v5.x),
            install from the Arduino Library Manager.
  Upload:   switch the shield's RX/serial switch OFF while uploading,
            then back ON to use it.
*/

#include <MIDI.h>

// ------------------------------------------------------------------ config
#define INPUT_CHANNEL  3    // channel the keyboard sends on
#define FIRST_OUT_CH   1    // first S2400 track channel
#define NUM_VOICES     8    // voices use channels FIRST_OUT_CH .. FIRST_OUT_CH+NUM_VOICES-1

#define PITCH_VIA_CC   1    // 1 = pitch as CC + fixed note, 0 = original note number
#define PITCH_CC       80   // CC that carries the note number (PITCH_VIA_CC = 1)
#define TRIGGER_NOTE   60   // note sent to trigger each track    (PITCH_VIA_CC = 1)
// -------------------------------------------------------------------------

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

const int8_t FREE = -1;
int8_t        voiceNote[NUM_VOICES];  // input note held on each voice, or FREE
unsigned long voiceAge[NUM_VOICES];   // when the voice was started (for stealing)
unsigned long ageCounter = 0;
uint8_t       nextVoice = 0;          // round-robin position

inline uint8_t voiceChannel(uint8_t v) { return FIRST_OUT_CH + v; }

void releaseVoice(uint8_t v) {
  if (voiceNote[v] == FREE) return;
#if PITCH_VIA_CC
  MIDI.sendNoteOff(TRIGGER_NOTE, 0, voiceChannel(v));
#else
  MIDI.sendNoteOff(voiceNote[v], 0, voiceChannel(v));
#endif
  voiceNote[v] = FREE;
}

int8_t findVoice(uint8_t note) {
  for (uint8_t v = 0; v < NUM_VOICES; v++)
    if (voiceNote[v] == (int8_t)note) return v;
  return -1;
}

// Next free voice in round-robin order; if all are busy, steal the oldest one.
uint8_t allocateVoice() {
  for (uint8_t i = 0; i < NUM_VOICES; i++) {
    uint8_t v = (nextVoice + i) % NUM_VOICES;
    if (voiceNote[v] == FREE) {
      nextVoice = (v + 1) % NUM_VOICES;
      return v;
    }
  }
  uint8_t oldest = 0;
  for (uint8_t v = 1; v < NUM_VOICES; v++)
    if (voiceAge[v] < voiceAge[oldest]) oldest = v;
  releaseVoice(oldest);
  nextVoice = (oldest + 1) % NUM_VOICES;
  return oldest;
}

void noteOn(uint8_t note, uint8_t velocity) {
  int8_t held = findVoice(note);   // same key hit again before its Note Off
  if (held >= 0) releaseVoice(held);

  uint8_t v  = allocateVoice();
  uint8_t ch = voiceChannel(v);
#if PITCH_VIA_CC
  MIDI.sendControlChange(PITCH_CC, note, ch);
  MIDI.sendNoteOn(TRIGGER_NOTE, velocity, ch);
#else
  MIDI.sendNoteOn(note, velocity, ch);
#endif
  voiceNote[v] = note;
  voiceAge[v]  = ++ageCounter;
}

void noteOff(uint8_t note) {
  int8_t v = findVoice(note);
  if (v >= 0) releaseVoice(v);
}

// Pass a message through unchanged (Thru is off, so nothing is sent twice).
void forward(midi::MidiType type, byte d1, byte d2, byte ch) {
  switch (type) {
    case midi::NoteOff:
    case midi::NoteOn:
    case midi::AfterTouchPoly:
    case midi::ControlChange:
    case midi::ProgramChange:
    case midi::AfterTouchChannel:
    case midi::PitchBend:
      MIDI.send(type, d1, d2, ch);
      break;
    case midi::SystemExclusive:
      MIDI.sendSysEx(MIDI.getSysExArrayLength(), MIDI.getSysExArray(), true);
      break;
    case midi::TimeCodeQuarterFrame: MIDI.sendTimeCodeQuarterFrame(d1);         break;
    case midi::SongPosition:         MIDI.sendSongPosition(d1 | (d2 << 7));    break;
    case midi::SongSelect:           MIDI.sendSongSelect(d1);                   break;
    case midi::TuneRequest:          MIDI.sendTuneRequest();                    break;
    case midi::Clock:
    case midi::Start:
    case midi::Continue:
    case midi::Stop:
    case midi::ActiveSensing:
    case midi::SystemReset:
      MIDI.sendRealTime(type);
      break;
    default:
      break;
  }
}

void setup() {
  for (uint8_t v = 0; v < NUM_VOICES; v++) {
    voiceNote[v] = FREE;
    voiceAge[v]  = 0;
  }
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();   // we forward everything ourselves
}

void loop() {
  if (!MIDI.read()) return;

  midi::MidiType type = MIDI.getType();
  byte ch = MIDI.getChannel();
  byte d1 = MIDI.getData1();
  byte d2 = MIDI.getData2();

  if (ch == INPUT_CHANNEL && type == midi::NoteOn && d2 > 0) {
    noteOn(d1, d2);
  } else if (ch == INPUT_CHANNEL &&
             (type == midi::NoteOff || (type == midi::NoteOn && d2 == 0))) {
    noteOff(d1);
  } else {
    forward(type, d1, d2, ch);
  }
}
