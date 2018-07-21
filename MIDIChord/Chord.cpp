#include "Chord.h"
#include "ListNode.h"

Chord::Chord(void) {
    this->head = new ListNode(-1, NULL, NULL);
    this->tail = new ListNode(-2, this->head, NULL);

    this->head->next = this->tail;
}

Chord::~Chord(void) {
}

int Chord::getSize() {
    int count = 0;
    ListNode* current;
    current = this->head->next;

    while (current != this->tail) {
        current = current->next;
        count++;
    }
    
    return count;
}

int Chord::getNote(int index) {
    ListNode* current;
    current = this->head->next;
    
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    
    return current->data;
}

int Chord::getHighestNote() {
    return this->tail->prev->data;
}

int Chord::getLowestNote() {
    return this->head->next->data;
}

byte Chord::getHighestNoteChannel() {
    return this->tail->prev->channel;
}

byte Chord::getLowestNoteChannel() {
    return this->tail->prev->channel;
}

byte Chord::getChannel(int index) {
    ListNode* current;
    current = this->head->next;

    for (int i = 0; i < index; i++) {
        current = current->next;
    }

    return current->channel;
}

void Chord::addNote(int tone) {
    ListNode* current;
    current = this->head->next;

    bool isFound = false;

    while((current != this->tail) && !isFound) {
        if (current->data < tone) {
            current = current->next;
        }
        else {
            isFound = true;
        }
    }

    ListNode* futurePrev = current->prev;
    ListNode* futureNext = current;
    futurePrev->next = new ListNode(tone, futurePrev, futureNext);
    futureNext->prev = futurePrev->next;
}

void Chord::addNote(int tone, byte channel) {
    ListNode* current;
    current = this->head->next;

    bool isFound = false;

    while((current != this->tail) && !isFound) {
        if (current->data < tone) {
            current = current->next;
        }
        else {
            isFound = true;
        }
    }

    ListNode* futurePrev = current->prev;
    ListNode* futureNext = current;
    futurePrev->next = new ListNode(tone, futurePrev, futureNext, channel);
    futureNext->prev = futurePrev->next;
}

uint8_t Chord::removeNote(int tone) {
    ListNode* current = this->head->next;
    bool isFound = false;
    uint8_t count = 0;

    while((current != this->tail) && !isFound) {
        if (current->data != tone) {
            current = current->next;
            count++;
        }
        else {
            isFound = true;
        }
    }

    if (isFound) {
        ListNode* futurePrev = current->prev;
        ListNode* futureNext = current->next;
        futurePrev->next = futureNext;
        futureNext->prev = futurePrev;

        delete current;
        current = NULL;
        return count;
    }
    return 255;
}

uint8_t Chord::removeNote(int tone, byte channel) {
    ListNode* current = this->head->next;
    bool isFound = false;
    uint8_t count = 0;

    while((current != this->tail) && !isFound) {
        if (current->data != tone && current->channel != channel) {
            current = current->next;
            count++;
        }
        else {
            isFound = true;
        }
    }

    if (isFound) {
        ListNode* futurePrev = current->prev;
        ListNode* futureNext = current->next;
        futurePrev->next = futureNext;
        futureNext->prev = futurePrev;

        delete current;
        current = NULL;
        return count;
    }
    return 255;
}

void Chord::removeChannel(byte channel) {
    ListNode* current = this->head->next;

    while((current != this->tail)) {
        if (current->channel == channel) {
            ListNode* futurePrev = current->prev;
            ListNode* futureNext = current->next;
            futurePrev->next = futureNext;
            futureNext->prev = futurePrev;

            delete current;
            current = futureNext;
        }
        else {
            current = current->next;
        }
    }
}

void Chord::clear() {
    ListNode* current = this->head->next;
    ListNode* next;
    while (current != this->tail) {
        next = current->next;
        delete current;
        current = next;
    }

    this->head->next = tail;
    this->tail->prev = head;
}
