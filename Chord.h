#ifndef CHORD_H
#define CHORD_H

#include "Arduino.h"
#include "ListNode.h"

class Chord {
public:
    Chord(void);
    ~Chord(void);
    unsigned int getSize();
    int getNote(int index, bool fromEnd = false);
    int getHighestNote();
    int getLowestNote();
    byte getHighestNoteChannel();
    byte getLowestNoteChannel();
    byte getChannel(int index, bool fromEnd = false);
    // void addNote(int tone);
    void addNote(int tone, byte channel);
    // bool removeNote(int tone);
    bool removeNote(int tone, byte channel);
    void removeChannel(byte channel);
    void clear();
private:
    ListNode* head;
    ListNode* tail;
    volatile unsigned int size;
};

#endif
