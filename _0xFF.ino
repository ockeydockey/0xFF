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
* [o] Modulation LFO
***************************************************/

// For MIDI
#include <MIDI.h>
#include <Chord.h>
#include <ListNode.h>
#include "utility.h"

// For Trellis
#include <Wire.h>
#include "Adafruit_Trellis.h"

// For Tone
//#include <NewTone.h>

/*******************************
* Hardware pin assignments
*******************************/
// Speaker output pin
#define SPEAKER 4
// Staccato enable switch
#define STACCATO 8
// Arpeggiation speed dial
#define ARP_SPEED A0
// Staccato gap length dial
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

MIDI_CREATE_DEFAULT_INSTANCE();

Chord chord;
uint8_t i;
unsigned int noteGap;
unsigned int noteLength;
unsigned long timer;
unsigned long int wait = millis() + 30;
int s;
byte c;
byte current[] = {255, 255};
boolean playing;
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

float bendCoefficient = 4096.0;
boolean pitchBendSemi = false;
uint8_t bendSemitones = 0;
uint8_t bendCents = 0;
float mod = 0;
float prevMod = 0;

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
    } else {
      HandleNoteOff(channel, pitch, velocity);
    }
  }
}

void HandleNoteOff(byte channel, byte pitch, byte velocity) {
  if (chord.removeNote(pitch, channel) <= i) {
    if (i > 0) {
      i--;
    } else { // accounts for counting errors
      i = 0;
    }
  }
  if (current[0] == pitch && current[1] == channel) {
    current[0] = 255;
    current[1] = 255;
  }
  if (chord.getSize() == 0) {
    noTone(SPEAKER);
  }
}

template<class State, byte MsbSelectCCNumber, byte LsbSelectCCNumber>
class ParameterNumberParser
{
public:
    ParameterNumberParser(State& inState)
        : mState(inState)
    {
    }

public:
    inline void reset()
    {
        mState.reset();
        mSelected = false;
        mCurrentNumber = 0;
    }

public:
    bool parseControlChange(byte inNumber, byte inValue)
    {
        switch (inNumber)
        {
            case MsbSelectCCNumber:
                mCurrentNumber.mMsb = inValue;
                break;
            case LsbSelectCCNumber:
                if (inValue == 0x7f && mCurrentNumber.mMsb == 0x7f)
                {
                    // End of Null Function, disable parser.
                    mSelected = false;
                }
                else
                {
                    mCurrentNumber.mLsb = inValue;
                    mSelected = mState.has(mCurrentNumber.as14bits());
                }
                break;

            case midi::DataIncrement:
                if (mSelected)
                {
                    Value& currentValue = getCurrentValue();
                    currentValue += inValue;
                    return true;
                }
                break;
            case midi::DataDecrement:
                if (mSelected)
                {
                    Value& currentValue = getCurrentValue();
                    currentValue -= inValue;
                    return true;
                }
                break;

            case midi::DataEntryMSB:
                if (mSelected)
                {
                    Value& currentValue = getCurrentValue();
                    currentValue.mMsb = inValue;
                    currentValue.mLsb = 0;
                    return true;
                }
                break;
            case midi::DataEntryLSB:
                if (mSelected)
                {
                    Value& currentValue = getCurrentValue();
                    currentValue.mLsb = inValue;
                    return true;
                }
                break;

            default:
                // Not part of the RPN/NRPN workflow, ignoring.
                break;
        }
        return false;
    }

public:
    inline Value& getCurrentValue()
    {
        return mState.get(mCurrentNumber.as14bits());
    }
    inline const Value& getCurrentValue() const
    {
        return mState.get(mCurrentNumber.as14bits());
    }

public:
    State& mState;
    bool mSelected;
    Value mCurrentNumber;
};

// --

typedef State<2> RpnState;  // We'll listen to 2 RPN
typedef State<4> NrpnState; // and 4 NRPN
typedef ParameterNumberParser<RpnState,  midi::RPNMSB,  midi::RPNLSB>  RpnParser;
typedef ParameterNumberParser<NrpnState, midi::NRPNMSB, midi::NRPNLSB> NrpnParser;

struct ChannelSetup
{
    inline ChannelSetup()
        : mRpnParser(mRpnState)
        , mNrpnParser(mNrpnState)
    {
    }

    inline void reset()
    {
        mRpnParser.reset();
        mNrpnParser.reset();
    }
    inline void setup()
    {
        mRpnState.enable(midi::RPN::PitchBendSensitivity);
        mRpnState.enable(midi::RPN::ModulationDepthRange);

        // Enable a few random NRPNs
        mNrpnState.enable(12);
        mNrpnState.enable(42);
        mNrpnState.enable(1234);
        mNrpnState.enable(1176);
    }

    RpnState    mRpnState;
    NrpnState   mNrpnState;
    RpnParser   mRpnParser;
    NrpnParser  mNrpnParser;
};

ChannelSetup sChannelSetup[16];

// --

void HandleControlChange(byte inChannel, byte inNumber, byte inValue) {
  ChannelSetup& channel = sChannelSetup[inChannel];

  if (inNumber == midi::ModulationWheel) {
    modDepth[inChannel] = ((float)inValue) / 255.0;
        
  } else if (channel.mRpnParser.parseControlChange(inNumber, inValue)) {
    const Value& value    = channel.mRpnParser.getCurrentValue();
    const unsigned number = channel.mRpnParser.mCurrentNumber.as14bits();

    if (number == midi::RPN::PitchBendSensitivity)
    {
      // Here, we use the LSB and MSB separately as they have different meaning.
      const byte semitones    = value.mMsb;
      const byte cents        = value.mLsb;
    }
    else if (number == midi::RPN::ModulationDepthRange)
    {
      // But here, we want the full 14 bit value.
      const unsigned range = value.as14bits();
    }
  }
  
//  // Handles the 4 message "Pitch Bend Range" RPN
//  if (pitchBendChecklist == 0 && number ==100 && value == 0) {
//    // maybe take off the "value == 0" this might have unforseen consequences
//    pitchBendChecklist |= 0x01;
//  } else if (pitchBendChecklist == 0x01 && number == 101 && value == 0) {
//    pitchBendChecklist |= 0x02;
//  } else if (pitchBendChecklist == 0x03 && number == 6) {
//    pitchBendChecklist |= 0x04;
//    // Calculate Pitch Bend Range in Semitones
//    bendCoefficient = 16384.0 / (float)value;
//    pitchBendChecklist = 0;
//  } /*else if (pitchBendChecklist == 0x07 && number == 38) {
//    pitchBendChecklist = 0;
//    // Add on cents to Pitch Bend Range
//    bendCoefficient += (float)(16384.0 / 100.0) * (float)value;
//  } fix later */
//  // handles other sequences of CC messages:
//  else if (pitchBendChecklist >= 0x01) {
//    pitchBendChecklist = 0;
//  }
}

void HandlePitchBend(byte channel, int amount) {
  //if (channel == 1) {
  // Yamaha XG compliant
  bend[channel] = ((float)amount) / bendCoefficient;
  // bend = ((float)amount) / 819.2;
  // bend = ((float)amount) / 767.0;
  // bend = ((float)amount) / 700;
  // bend = ((float)amount) / 660.0;
  // Calibrated to (Voodoo - Jimmy Hendrix - XG.mid).  This should be true for all Yamaha XG.
  // bend = ((float)amount) / 655.0;
  //}
}

inline float calcModulation(byte channel) {
  // Implements a single instruction byte mask to replace a more costly modulus of 256
  // Using 1 step per 2 milliseconds on a 256 step sine wave gives a period of 256/1000 of a second (or about 1/4 second)
  // Adjust the milliseconds to degrees, then convert to radians by multiplying by 0.0174533
  return sin(0.0174533 * ((360 * ((millis() & 0xFE)) >> 1) / 256)) * modDepth[ channel ];
}

// Lowest note priority (monophonic)
void playLowest() {
  s = chord.getSize();
  c = chord.getLowestNoteChannel();
  if (s > 0 && chord.getLowestNote() > 0) {
    tone(
      SPEAKER,
      440.0 * pow(2.0, (chord.getLowestNote() - 69.0 + bend[c] + calcModulation(c)) / 12.0)
    );
    timer = millis();
  } else if (s == 0) {
    noTone(SPEAKER);
  }
}

// Highest note priority (monophonic)
void playHighest() {
  s = chord.getSize();
  c = chord.getHighestNoteChannel();

  if (s > 0 && chord.getHighestNote() > 0) {
    tone(
      SPEAKER,
      440.0 * pow(2.0, (chord.getHighestNote() - 69.0 + bend[c] + calcModulation(c)) / 12.0)
    );
    timer = millis();
  } else if (s == 0) {
    noTone(SPEAKER);
  }
}  

void arpAscend() {
  bool change = false;
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
      tone(
        SPEAKER,
        440.0 * pow(2.0, (chord.getNote(i) - 69.0 + bend[c] + mod) /12.0)
      );
    }
  }

  if (playing && s != 1 && ((long)(millis() - timer) >= 0)) {
    noTone(SPEAKER);
    // Set timeout for gap between notes
    timer = millis + noteGap;

    // Prepare to play next note
    playing = false;
    i++;
  }

  // Update previous values
  prevbend[c] = bend[c];
  prevMod = mod;
}

void arpDescend() {
  bool change = false;
  s = chord.getSize();
  // If the current note index is bigger than the chord size
  if (i >= s) {
    i = 0;
  }
  c = chord.getChannel(i, true);
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
      tone(
        SPEAKER,
        440.0 * pow(2.0, (chord.getNote(i, true) - 69.0 + bend[c] + mod) /12.0)
      );
    }
  }

  if (playing && s != 1 && ((long)(millis() - timer) >= 0)) {
    noTone(SPEAKER);
    // Set timeout for gap between notes
    timer = millis + noteGap;

    // Prepare to play next note
    playing = false;
    i++;
  }

  // Update previous values
  prevbend[c] = bend[c];
  prevMod = mod;
}

// Most recent note priority (monophonic)
void playCurrent() {
  if (current[0] < 255 && current[1] < 255) {
    tone(
        SPEAKER,
        440.0 * pow(2.0, (current[0] - 69.0 + bend[current[1]] + calcModulation(current[1])) / 12.0)
      );
    timer = millis();
  } else {
    noTone(SPEAKER);
  }
}

void setup() { 
  i = 0;
  timer = 0;
  s = 0;
  playing = false;
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
