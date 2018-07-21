#include "ListNode.h"

ListNode::ListNode(void) {
    this->next = NULL;
    this->prev = NULL;
}

ListNode::ListNode(int data, ListNode* prev, ListNode* next) {
    this->data = data;
    this->prev = prev;
    this->next = next;
}

ListNode::ListNode(int data, ListNode* prev, ListNode* next, byte channel) {
    this->data = data;
    this->prev = prev;
    this->next = next;
    this->channel = channel;
}

ListNode::~ListNode(void) {
    // This page intentionally left blank
    //                                                - IBM
}
