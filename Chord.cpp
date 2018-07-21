#include "Chord.h"
#include "ListNode.h"

Chord::Chord(void) {
    this->head = new ListNode(-1, NULL, NULL);
    this->tail = new ListNode(-2, this->head, NULL);

    this->head->next = this->tail;

    this->size = 0;
}

Chord::~Chord(void) {
    this->clear();
}

unsigned int Chord::getSize() {
    return this->size;
}

int Chord::getNote(int index, bool fromEnd) {
    ListNode* current;

    if (fromEnd) {
        current = this->tail->prev;

        for (; index > 0; index--) {
            current = current->prev;
        }

    } else {
        current = this->head->next;
        
        for (; index > 0; index--) {
            current = current->next;
        }
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

byte Chord::getChannel(int index, bool fromEnd) {
    ListNode* current;
    
    if (fromEnd) {
        current = this->tail->prev;

        for (; index > 0; index--) {
            current = current->prev;
        }

    } else {
        current = this->head->next;

        for (; index > 0; index--) {
            current = current->next;
        }
    }

    return current->channel;
}

// void Chord::addNote(int tone) {
//     ListNode* current;
//     current = this->head->next;

//     bool isFound = false;

//     while((current != tail) && !isFound) {
//         if (current->data < tone) {
//             current = current->next;
//         }
//         else {
//             isFound = true;
//         }
//     }

//     ListNode* futurePrev = current->prev;
//     ListNode* futureNext = current;
//     futurePrev->next = new ListNode(tone, futurePrev, futureNext);
//     futureNext->prev = futurePrev->next;

//     size++;
// }

void Chord::addNote(int tone, byte channel) {
    ListNode* current;
    current = this->head->next;

    bool isFound = false;

    while((current != tail) && !isFound) {
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

    size++;
}

// bool Chord::removeNote(int tone) {
//     ListNode* current = this->head->next;
//     bool isFound = false;

//     while(!isFound && (current != tail)) {
//         if (current->data == tone) {
//             isFound = true;
//         }
//         else {
//             current = current->next;
//         }
//     }

//     if (isFound) {
//         ListNode* futurePrev = current->prev;
//         ListNode* futureNext = current->next;
//         futurePrev->next = futureNext;
//         futureNext->prev = futurePrev;

//         delete current;
//         current = NULL;

//         size--;
//     }

//     return isFound;
// }

bool Chord::removeNote(int tone, byte channel) {
    ListNode* current = this->head->next;
    bool isFound = false;

    // while((current != this->tail) && !isFound) {
    while(!isFound && (current != tail)) {
        if (current->data == tone && current->channel == channel) {
            isFound = true;
        } else {
            current = current->next;
        }
    }

    if (isFound) {
        ListNode* futurePrev = current->prev;
        ListNode* futureNext = current->next;
        futurePrev->next = futureNext;
        futureNext->prev = futurePrev;

        delete current;
        current = NULL;

        size--;
    }

    return isFound;
}

void Chord::removeChannel(byte channel) {
    ListNode* current = this->head->next;

    while((current != tail)) {
        if (current->channel == channel) {
            ListNode* futurePrev = current->prev;
            ListNode* futureNext = current->next;
            futurePrev->next = futureNext;
            futureNext->prev = futurePrev;

            delete current;
            current = futureNext;

            size--;
        }
        else {
            current = current->next;
        }
    }
}

void Chord::clear() {
    ListNode* current = this->head->next;
    ListNode* next;
    while (current != tail) {
        next = current->next;
        delete current;
        current = next;
    }

    this->head->next = tail;
    this->tail->prev = head;
    this->size = 0;
}
