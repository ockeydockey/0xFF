/***************************************************
* Created by David Ockey
* (C) 2011 - 2018
****************************************************
* Uses Arduino MIDI libary 4.3.1, by FortySevenEffects
* 
* Features:
* [ ] 16 Channel selector button matrix
* [ ] 5 arpeggiation modes
* [ ] Adjustable arpeggiation
* [ ] Adjustable staccato mode for arpeggiation modes
* [ ] Per-channel pitch bending
* [o] Adjustable pitch bend according to GM spec
* [ ] Modulation LFO
* 
* Requires:
* [ ] Arduino MIDI Library 4.3.1 (by FortySevenEffects)
* [ ] Adafruit Trellis Library (Tested with Versions 1.0.0 and 1.0.1)
* [ ] Either: Arduino Tone Library OR NewTone Library (by Tim Eckel)
***************************************************/

// For MIDI
#include <MIDI.h>
#include "RPN_Parser.cpp"
#include "Chord.h"
#include "Portamento.h"
#include "utility.h"

// For Trellis
#include <Wire.h>
#include "Adafruit_Trellis.h"

// For LFO
#include "LFOWaveforms.h"

// For Tone
//#include <NewTone.h>

/*******************************
* Hardware pin assignments
*******************************/
// Speaker output pin
#define SPEAKER 4
// Staccato enable switch
#define AUX_ENABLE 8
// Arpeggiation speed dial
#define ARP_SPEED A0
// Staccato gap length dial
#define AUX_DIAL A1

/*******************************
* Arpeggiation modes
*******************************/
// Low Priority
#define AMODE_LOWEST 9
// Ascending Arpeggiation
#define AMODE_ASCEND 10
// Play most recent note
#define AMODE_OFF 11
// Descending Arpeggiation
#define AMODE_DESCEND 12
// High Priority
#define AMODE_HIGHEST 13

// Initialize MIDI Library instance
MIDI_CREATE_DEFAULT_INSTANCE();

typedef struct note_t {
  byte note;
  byte channel;
} Note;

typedef struct sensitivity_t {
  byte semitones;
  byte cents;
} Sensitivity;

// Note storage
Chord chord;

// Settings
uint32_t noteGap;
uint32_t noteLength;
Sensitivity bendSensitivity;

// Current conditions
unsigned long timer;
unsigned long int trellisUpdateTimeout = millis() + 30;
uint8_t i;
volatile unsigned int s;
volatile byte c;
volatile bool change = false;
Note current = { .note = 0xFF, .channel = 0xFF };
boolean playing;
float mod = 0;
float prevMod = 0;
ChannelSetup sChannelSetup[16];

boolean chanEnable[] = {
  false, // There are 17 indexes to make 1-16 available for each MIDI channel.  This saves on math.
  true,  false, false, false,
  false, false, false, false,
  false, false, false, false,
  false, false, false, false
};

// 16 channels of pitch bend values
float bend[] = {
  0, // There are 17 indexes to make 1-16 available for each MIDI channel.  This saves on math.
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

// 16 channels of previous bend values to know if the CC has changed
float prevbend[] = {
  0, // There are 17 indexes to make 1-16 available for each MIDI channel.  This saves on math.
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

float modDepth[] = {
  0, // There are 17 indexes to make 1-16 available for each MIDI channel.  This saves on math.
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis = Adafruit_TrellisSet(&matrix0);

// Callback for MIDI PitchBend Control Change message
void HandlePitchBend(byte channel, int amount) {
  // The amount returned from the MIDI library is a zero-centered 14-bit int (zero == no bend)
  // bend[channel] = ((float)((int16_t)(amount - 8192)) / 8192.0) * ((float)bendSensitivity.semitones + (0.01 * bendSensitivity.cents));
  bend[channel] = ((float)((float)(amount)) / 8192.0f) * ((float)bendSensitivity.semitones + (0.01 * bendSensitivity.cents));
}

inline float calcModulation(byte channel) {
  // Implements a single instruction byte mask to replace a more costly modulus of 256
  // Using 1 step per millisecond on a 256 step sine wave gives a period of 256/1000 of a second (or about 1/4 second)
  return pgm_read_float(&lfo_sine256[ (millis() & 0xFF) ]) * modDepth[ channel ];
}

inline void playNote(int note, byte channel) { // inline used to reduce instruction/stack overhead for function
#ifdef NewTone_h
  NewTone(
#else
  tone(
#endif
    SPEAKER,
    440.0 * pow(2.0, ((note - 69.0) + bend[channel] + calcModulation(channel) + Portamento_getCurrentOffset()) / 12.0)
  );
}

inline void stopNote() { // inline used to reduce instruction/stack overhead for function
#ifdef NewTone_h
  noNewTone(SPEAKER);
#else
  noTone(SPEAKER);
#endif
}

// Update the chanEnable statuses with the current Adafruit Trellis button states
void updateChannels() {
  for (uint8_t i = 0; i < 16; i++) {
    if (trellis.justPressed(i)) {
      chanEnable[i + 1] = !chanEnable[i + 1];
      if(chanEnable[i + 1]) {
        trellis.setLED(i);
      } else {
        chord.removeChannel(i + 1);
        trellis.clrLED(i);
      }
    }
  }
  trellis.writeDisplay();
}

// Callback for MIDI NoteON message
void HandleNoteOn(byte channel, byte pitch, byte velocity) {
  if (chanEnable[channel]) {
    if (velocity != 0) {
      chord.addNote(pitch, channel);
      if (digitalRead(AUX_ENABLE)) {
        Portamento_startGlide(current.note, (uint8_t)pitch, analogRead(AUX_DIAL));
      }
      current.note = pitch;
      current.channel = channel;
    } else {
      HandleNoteOff(channel, pitch, velocity);
    }
  }
}

// Callback for MIDI NoteOFF message
void HandleNoteOff(byte channel, byte pitch, byte velocity) {
  chord.removeNote(pitch, channel); // returns bool if debugging is needed

  if (current.note == pitch && current.channel == channel) {
    current.note = 255;
    current.channel = 255;
    Portamento_stopGlide();
  }
  
  if (chord.getSize() == 0) {
    stopNote();
  }

  change = true;
}

void HandleControlChange(byte inChannel, byte inNumber, byte inValue) {
  ChannelSetup& channel = sChannelSetup[inChannel];

  if (inNumber == midi::ModulationWheel) {
    // Modulation Wheel has a range of 0 - 127 (according to MIDI spec)
    modDepth[inChannel] = ((float)inValue) / 256.0;
  } else if (channel.mRpnParser.parseControlChange(inNumber, inValue)) {
    const Value& value    = channel.mRpnParser.getCurrentValue();
    
    switch(channel.mRpnParser.mCurrentNumber.as14bits()) {
    case midi::RPN::PitchBendSensitivity:
      // Here, we use the LSB and MSB separately as they have different meaning.
      bendSensitivity.semitones    = value.mMsb;
      bendSensitivity.cents        = value.mLsb;
      break;
    
    case midi::RPN::ModulationDepthRange:
      // But here, we want the full 14 bit value.
      const unsigned range = value.as14bits();
      break;
    }
  }
}

// Lowest note priority (monophonic)
void playLowest() {
  s = chord.getSize();
  c = chord.getLowestNoteChannel();
  if (s > 0 && chord.getLowestNote() >= 0) {
    playNote(chord.getLowestNote(), c);
    timer = millis();
  } else if (s == 0) {
    stopNote();
  }
  change = false;
}

// Highest note priority (monophonic)
void playHighest() {
  s = chord.getSize();
  c = chord.getHighestNoteChannel();

  if (s > 0 && chord.getHighestNote() >= 0) {
    playNote(chord.getHighestNote(), c);
    timer = millis();
  } else if (s == 0) {
    stopNote();
  }
  change = false;
}  

void arpAscend() {
  Portamento_stopGlide();
  s = chord.getSize();
  // If the current note index is bigger than the chord size
  if (i >= s) {
    i = 0;
  }
  c = chord.getChannel(i);
  mod = calcModulation(c);

  // If the chord size is greater than 0
  if (s > 0) {
    // If not palying a note, and the note gap timer is up
    if (!playing && ((long)(millis() - timer) >= 0)) {
      // Set timeout for current note duration
      timer = millis() + noteLength;
      playing = true;
      change = true;
    } else if (prevbend[c] != bend[c]) {
      change = true;
    } else if (prevMod != mod) {
      change = true;
    }
  
    if (playing && change) {
      playNote(chord.getNote(i), c);
    }
  } else {
    stopNote();
  }

  if (playing && s != 1 && ((long)(millis() - timer) >= 0)) {
    stopNote();
    
    // Set timeout for gap between notes
    timer = millis() + noteGap;

    // Prepare to play next note
    playing = false;
    i++;
  }

  // Update previous values
  prevbend[c] = bend[c];
  prevMod = mod;
  change = false;
}

void arpDescend() {
  Portamento_stopGlide();
  s = chord.getSize();
  // If the current note index is bigger than the chord size
  if (i >= s) {
    i = 0;
  }
  c = chord.getChannel(i, true);
  mod = calcModulation(c);

  // If the chord size is greater than 0
  if (s > 0) {
    // If not playing a note, and the note gap timer is up
    if (!playing && ((long)(millis() - timer) >= 0)) {
      // Set timeout for current note duration
      timer = millis() + noteLength;
      playing = true;
      change = true;
    } else if (prevbend[c] != bend[c]) {
      change = true;
    } else if (prevMod != mod) {
      change = true;
    }
  
    if (playing && change) {
      playNote(chord.getNote(i, true), c);
    }
  }

  if (playing && s != 1 && ((long)(millis() - timer) >= 0)) {
    stopNote();
    
    // Set timeout for gap between notes
    timer = millis() + noteGap;

    // Prepare to play next note
    playing = false;
    i++;
  }

  // Update previous values
  prevbend[c] = bend[c];
  prevMod = mod;
  change = false;
}

// Most recent note priority (monophonic)
void playCurrent() {
  if (current.note < 255 && current.channel < 255) {
    playNote(current.note, current.channel);
    timer = millis();
  } else {
    stopNote();
  }
  change = false;
}

void setup() { 
  i = 0;
  timer = 0;
  s = 0;
  playing = false;

  bendSensitivity.semitones = 1;
  bendSensitivity.cents = 0;

  // Enable output of MIDI optoisolator
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);

  // Set pin high for Trellis Interrupt pin
  pinMode(A2, INPUT);
  digitalWrite(A2, HIGH);

  // Set the input for the arpeggiation speed and note Gap
  pinMode(ARP_SPEED, INPUT);
  pinMode(AUX_DIAL, INPUT);
  pinMode(AUX_ENABLE, INPUT_PULLUP);

  // Set the pinmode to PULLUP for the 5 way selector
  pinMode(AMODE_LOWEST, INPUT_PULLUP);
  pinMode(AMODE_ASCEND, INPUT_PULLUP);
  pinMode(AMODE_OFF, INPUT_PULLUP);
  pinMode(AMODE_DESCEND, INPUT_PULLUP);
  pinMode(AMODE_HIGHEST, INPUT_PULLUP);
  
  // Set MIDI handlers
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);
  MIDI.setHandlePitchBend(HandlePitchBend);
  MIDI.setHandleControlChange(HandleControlChange);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  trellis.begin(0x70);
  for (uint8_t i = 0; i < 16; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();
    delay(40);
  }
  delay(100);
  for (uint8_t i = 0; i < 16; i++) {
    if (chanEnable[i + 1]) {
      trellis.setLED(i);
    } else {
      trellis.clrLED(i);
    }
    trellis.writeDisplay();
    delay(40);
  }
}

void loop() {
  if (millis() >= trellisUpdateTimeout) {
    trellisUpdateTimeout = millis() + 30;
    if (trellis.readSwitches()) {
      updateChannels();
    }
  }
  MIDI.read();
  if (digitalRead(AUX_ENABLE)) {
    noteLength = map(analogRead(ARP_SPEED), 0, 1024, 10, 500);
    // Carve out the gap from the total note length.  (Preserves musical timing)
    noteGap = map(analogRead(AUX_DIAL), 0, 1024, 1, (noteLength * 10) / 9);
    noteLength = noteLength - noteGap;
  } else {
    noteLength = map(analogRead(ARP_SPEED), 0, 1024, 10, 500);
    noteGap = 0;
  }

  // Read mode selector pins
  if (digitalRead(AMODE_LOWEST) == LOW) {
    playLowest();
  } else if (digitalRead(AMODE_ASCEND) == LOW) {
    arpAscend();
  } else if (digitalRead(AMODE_OFF) == LOW) {
    playCurrent();
  } else if (digitalRead(AMODE_DESCEND) == LOW) {
    arpDescend();
  } else if (digitalRead(AMODE_HIGHEST) == LOW) {
    playHighest();
  } else {
    stopNote();
  }
}
