/***************************************************
* Created by David Ockey
* (C) 2011 - 2015
****************************************************
* Features:
* [ ] 16 Channel selector button matrix
* [ ] 5 arpeggiation modes
* [ ] Adjustable arpeggiation
* [ ] Per-channel pitch bending
* [o] Adjustable pitch bend according to GM spec
***************************************************/

// For MIDI
#include <MIDI.h>
#include <Chord.h>
#include <ListNode.h>

// For Trellis
#include <Wire.h>
#include "Adafruit_Trellis.h"

// Hardware assignments
#define SPEAKER 4
#define STACCATO 8
#define ARP_SPEED A0
#define GAP A1

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

Chord chord;
uint8_t i;
unsigned int noteGap;
unsigned int noteLength;
unsigned long timer;
unsigned long int wait = millis() + 30;
int s;
byte c;
byte current[] = {255, 255};
boolean play;
boolean chanEnable[] = {
  false,
  true,    // There are 17 indexes to make 1-16 available.  This saves on math.
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false
};
double bend[] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};
double prevbend[] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};
double bendCoefficient = 4096.0;
//boolean pitchBendMSB = false;
//boolean pitchBendLSB = false;
byte pitchBendChecklist = 0;
boolean pitchBendSemi = false;
uint8_t bendSemitones = 0;
uint8_t bendCents = 0;

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis = Adafruit_TrellisSet(&matrix0);

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

void HandleNoteOn(byte channel, byte pitch, byte velocity) {
  if (chanEnable[channel]) {
    if (velocity != 0) {
      chord.addNote((int)pitch, channel);
      current[0] = pitch;
      current[1] = channel;
    }
    else {
      HandleNoteOff(channel, pitch, velocity);
    }
  }
}

void HandleNoteOff(byte channel, byte pitch, byte velocity) {
  if (chord.removeNote(pitch) <= i) {
    if (i > 0) {
      i--;
    } else {
      i = 0;
    }
  }
  if (current[0] == pitch && current[1] == channel) {
    current[0] = 255;
    current[1] = 255;
  }
  if (chord.getSize() == 0) {
    noTone(SPEAKER);
    play = true;
  }
}

void HandleControlChange(byte channel, byte number, byte value) {
  // Handles the 4 message "Pitch Bend Range" RPN
  if (pitchBendChecklist == 0 && number ==100 && value == 0) {
    // maybe take off the "value == 0" this might have unforseen consequences
    pitchBendChecklist |= 0x01;
  } else if (pitchBendChecklist == 0x01 && number == 101 && value == 0) {
    pitchBendChecklist |= 0x02;
  } else if (pitchBendChecklist == 0x03 && number == 6) {
    pitchBendChecklist |= 0x04;
    // Calculate Pitch Bend Range in Semitones
    bendCoefficient = 16384.0 / (double)value;
    pitchBendChecklist = 0;
  } /*else if (pitchBendChecklist == 0x07 && number == 38) {
    pitchBendChecklist = 0;
    // Add on cents to Pitch Bend Range
    bendCoefficient += (double)(16384.0 / 100.0) * (double)value;
  } fix later */
  // handles other sequences of CC messages:
  else if (pitchBendChecklist >= 0x01) {
    pitchBendChecklist = 0;
  }
  
  
//    if (number == 100 && value == 0) {
//      pitchBendMSB = true;
//      bendSemitones = 0;
//    }
//    else if (number == 101 && value == 0 && pitchBendMSB) {
//      pitchBendLSB = true;
//      pitchBendMSB = false;
//    }
//    else if (number == 6 && pitchBendLSB) {
//      bendCoefficient = 16384.0 / (double)value;
//    }
//    else if (number == 38 && bendSemitones != 0) {
//      bendCoefficient = 16384.0 / ((double)bendSemitones + ((double)bendCents / 100.0));
//      bendSemitones = 0;
//      bendCents = 0;
//    }
//    else {
//      pitchBendMSB = false;
//      pitchBendLSB = false;
//      bendSemitones = 0;
//      bendCents = 0;
//    }
}

void HandlePitchBend(byte channel, int amount) {
  //if (channel == 1) {
  // Yamaha XG compliant
   bend[channel] = ((double)amount) / bendCoefficient;
  // bend = ((double)amount) / 819.2;
  // bend = ((double)amount) / 767.0;
  // bend = ((double)amount) / 700;
  // bend = ((double)amount) / 660.0;
  // Calibrated to (Voodoo - Jimmy Hendrix - XG.mid).  This should be true for all Yamaha XG.
  // bend = ((double)amount) / 655.0;
  //}
}

void playLowest() {
  s = chord.getSize();
  c = chord.getLowestNoteChannel();
  if (s > 0 && chord.getLowestNote() > 0) {
    tone(
      SPEAKER,
      440.0 * pow(2.0, (chord.getLowestNote() - 69.0 + bend[c]) / 12.0)
    );
    timer = millis();
  } else if (s == 0) {
    noTone(SPEAKER);
  }
}

void playHighest() {
  s = chord.getSize();
  c = chord.getHighestNoteChannel();

  if (s > 0 && chord.getHighestNote() > 0) {
    tone(
      SPEAKER,
      440.0 * pow(2.0, (chord.getHighestNote() - 69.0 + bend[c]) / 12.0)
    );
    timer = millis();
  } else if (s == 0) {
    noTone(SPEAKER);
  }
}  

void arpAscend() {
  s = chord.getSize();
  c = chord.getChannel(i);
  if (prevbend[c] != bend[c] && s == 1) {
    // noTone(SPEAKER);
    play = true;
  }
  if (s > 0 && play) {
    if (millis() - timer > noteGap) {
      tone(
        SPEAKER,
        440.0 * pow(2.0, (chord.getNote(i) - 69.0 + bend[c]) / 12.0)
      );
      play = !play;
      i++;
      i = i % s;
      timer = millis();
    }
  }
  if ((!play && millis() - timer > noteLength) && s != 1) {
    noTone(SPEAKER);
    play = !play;
    timer = millis();
  }
  prevbend[c] = bend[c];
}

void arpDescend() {
  s = chord.getSize();
  if (i > s) {
    i = s;
  }
  c = chord.getChannel(i);
  if (prevbend[c] != bend[c] && s == 1) {
    // noTone(SPEAKER);
    play = true;
  }
  if (s > 0 && play) {
    if (millis() - timer > noteGap) {
      tone(
        SPEAKER,
        440.0 * pow(2.0, (chord.getNote(i) - 69.0 + bend[c]) / 12.0)
      );
      play = !play;
      i--;
      if (i < 0) {
        //i = i % s;
        i = s;
      }
      timer = millis();
    }
  }
  if ((!play && millis() - timer > noteLength) && s != 1) {
    noTone(SPEAKER);
    play = !play;
    timer = millis();
  }
  prevbend[c] = bend[c];
}

void playCurrent() {
  if (current[0] < 255 && current[1] < 255) {
    tone(
        SPEAKER,
        440.0 * pow(2.0, (current[0] - 69.0 + bend[current[1]]) / 12.0)
      );
    timer = millis();
  } else {
    noTone(SPEAKER);
  }
}

void setup() { 
  MIDI.begin(MIDI_CHANNEL_OMNI);
  i = 0;
  timer = 0;
  s = 0;
  play = true;
  // Set pin high for Trellis Interrupt pin
  pinMode(A2, INPUT);
  digitalWrite(A2, HIGH);
  // Set the input for the arpeggiation speed and note Gap
  pinMode(ARP_SPEED, INPUT);
  pinMode(GAP, INPUT);
  pinMode(STACCATO, INPUT_PULLUP);
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
  if (millis() >= wait) {
    wait = millis() + 30;
    if (trellis.readSwitches()) {
      updateChannels();
    }
  }
  MIDI.read();
  if (digitalRead(STACCATO)) {
    noteLength = map(analogRead(ARP_SPEED),0, 1024, 10, 500);
    // Carve out the gap from the total note length.  (Preserves musical timing)
    noteGap = map(analogRead(GAP),0, 1024, 1, (noteLength * 10) / 9);
    noteLength = noteLength - noteGap;
  } else {
    noteLength = map(analogRead(ARP_SPEED),0, 1024, 10, 500);
    noteGap = 0;
  }

  if (digitalRead(AMODE_LOWEST) == LOW) {
    playLowest();
  } else if (digitalRead(AMODE_ASCEND) == LOW) {
    arpAscend();
  } else if (digitalRead(AMODE_OFF) ==LOW) {
    playCurrent();
  } else if (digitalRead(AMODE_DESCEND) == LOW) {
    arpDescend();
  } else if (digitalRead(AMODE_HIGHEST) == LOW) {
    playHighest();
  } else {
    noTone(SPEAKER);
  }
}
