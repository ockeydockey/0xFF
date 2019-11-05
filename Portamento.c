#include "Portamento.h"
#include "Arduino.h"

// Variables used by the state machine
static float glideIncrement = 0.0;
static uint32_t startTime = 0L;
static uint16_t glideDuration = 1000L;
static bool isGliding = false;

// Returns true when 
void Portamento_startGlide(uint8_t srcNote, uint8_t dstNote, uint16_t duration) {
    // Check if previous note is valid
    if (srcNote != 0xFF) {
        // This is reversed so that glide can use the destination note as the reference point
        int8_t distance = srcNote - dstNote;
        glideDuration = duration;
        glideIncrement = (float)((float)distance) / (float)(glideDuration);
        isGliding = true;
        startTime = millis();
    }
}

void Portamento_stopGlide() {
    isGliding = false;
}

// TODO use this function to produce the offset in the playNote function!!!!
float Portamento_getCurrentOffset() {
    if (!isGliding) {
        return 0.0;
    } else {
        float result;
        // TODO check time
        uint32_t timeElapsed = millis() - startTime;
        if (timeElapsed >= glideDuration) {
            isGliding = false;
            result = 0.0;
        } else {
            result = (float)(glideIncrement * (float)(glideDuration - timeElapsed));
        }

        return result;
    }
}
