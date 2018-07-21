#ifndef ListNode_h
#define ListNode_h

#include "Arduino.h"

class ListNode {
    public:
        int data;
        ListNode* next;
        ListNode* prev;
        byte channel;
        
        ListNode(void);
        ListNode(int data, ListNode* prev, ListNode* next);
        ListNode(int data, ListNode* prev, ListNode* next, byte channel);
        ~ListNode(void);
};

#endif
