#include <MIDI.h>
#include "utility.h"

template<class State, byte MsbSelectCCNumber, byte LsbSelectCCNumber>
class ParameterNumberParser
{
public:
    ParameterNumberParser(State& inState)
        : mState(inState) {}

public:
    inline void reset() {
        mState.reset();
        mSelected = false;
        mCurrentNumber = 0;
    }

public:
    bool parseControlChange(byte inNumber, byte inValue) {
        switch (inNumber) {
            case MsbSelectCCNumber:
                mCurrentNumber.mMsb = inValue;
                break;
            case LsbSelectCCNumber:
                if (inValue == 0x7f && mCurrentNumber.mMsb == 0x7f) {
                    // End of Null Function, disable parser.
                    mSelected = false;
                } else {
                    mCurrentNumber.mLsb = inValue;
                    mSelected = mState.has(mCurrentNumber.as14bits());
                }
                break;

            case midi::DataIncrement:
                if (mSelected) {
                    Value& currentValue = getCurrentValue();
                    currentValue += inValue;
                    return true;
                }
                break;
            case midi::DataDecrement:
                if (mSelected) {
                    Value& currentValue = getCurrentValue();
                    currentValue -= inValue;
                    return true;
                }
                break;

            case midi::DataEntryMSB:
                if (mSelected) {
                    Value& currentValue = getCurrentValue();
                    currentValue.mMsb = inValue;
                    currentValue.mLsb = 0;
                    return true;
                }
                break;
            case midi::DataEntryLSB:
                if (mSelected) {
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
    inline Value& getCurrentValue() {
        return mState.get(mCurrentNumber.as14bits());
    }
    
    inline const Value& getCurrentValue() const {
        return mState.get(mCurrentNumber.as14bits());
    }

public:
    State& mState;
    bool mSelected;
    Value mCurrentNumber;
};

// --

typedef State<2> RpnState;  //Listen to 2 RPNs
typedef ParameterNumberParser<RpnState,  midi::RPNMSB,  midi::RPNLSB>  RpnParser;

struct ChannelSetup {
    inline ChannelSetup()
        : mRpnParser(mRpnState) {}

    inline void reset() {
        mRpnParser.reset();
    }
    
    inline void setup() {
        mRpnState.enable(midi::RPN::PitchBendSensitivity);
        mRpnState.enable(midi::RPN::ModulationDepthRange);
    }

    RpnState    mRpnState;
    RpnParser   mRpnParser;
};

// --
