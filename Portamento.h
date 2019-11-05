#include "Arduino.h"

#ifndef _0XFF_PORTAMENTO_H
#define _0XFF_PORTAMENTO_H

#ifdef __cplusplus
extern "C" {
#endif

void Portamento_startGlide(uint8_t srcNote, uint8_t dstNote, uint16_t duration);
void Portamento_stopGlide();
float Portamento_getCurrentOffset();

#ifdef __cplusplus
}
#endif

#endif
