#ifndef CHORD_H
#define CHORD_H

#include "Arduino.h"
#include "ListNode.h"

class Chord {
public:
    Chord(void);
    ~Chord(void);
    int getSize();
    int getNote(int index);
    int getHighestNote();
    int getLowestNote();
    byte getHighestNoteChannel();
    byte getLowestNoteChannel();
    byte getChannel(int index);
    void addNote(int tone);
    void addNote(int tone, byte channel);
    uint8_t removeNote(int tone);
    uint8_t removeNote(int tone, byte channel);
    void removeChannel(byte channel);
    void clear();
private:
    ListNode* head;
    ListNode* tail;
    byte channel;
};

#endif
