#include "client.h"

void LinkAppend(LinkNode **head, void *data) {
    LinkNode *node = malloc(sizeof(LinkNode));
    if (node == NULL) {
        MemoryError("LinkAppend", "Failed to allocate memory for node");
        return;
    }
    node->data = data;
    node->next = NULL;
    if (*head == NULL) {
        *head = node;
        return;
    }
    LinkNode *cur = *head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = node;
}

void LinkDeleteData(LinkNode **head, void *data) {
    LinkNode *cur = *head;
    if (cur == NULL) return;
    if (cur->data == data) {
        *head = cur->next;
        free(cur);
        return;
    }
    while (cur->next != NULL) {
        if (cur->next->data == data) {
            LinkNode *next = cur->next;
            cur->next = next->next;
            free(next);
            return;
        }
        cur = cur->next;
    }
}

void LinkRelease(LinkNode **head) {
    LinkNode *cur = *head;
    while (cur != NULL) {
        LinkNode *next = cur->next;
        free(cur);
        cur = next;
    }
    *head = NULL;
}