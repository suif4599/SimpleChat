#include "link.h"

int LinkAppend(LinkNode **head, void *data) {
    LinkNode *node = malloc(sizeof(LinkNode));
    if (node == NULL) {
        MemoryError("LinkAppend", "Failed to allocate memory for node");
        return -1;
    }
    node->data = data;
    node->next = NULL;
    if (*head == NULL) {
        *head = node;
        return 0;
    }
    LinkNode *cur = *head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = node;
    return 0;
}

int LinkDeleteData(LinkNode **head, void *data) {
    LinkNode *cur = *head;
    if (cur == NULL) return -1;
    if (cur->data == data) {
        *head = cur->next;
        free(cur);
        return 0;
    }
    while (cur->next != NULL) {
        if (cur->next->data == data) {
            LinkNode *next = cur->next;
            cur->next = next->next;
            free(next);
            return 0;
        }
        cur = cur->next;
    }
    return -1;
}

LinkNode* LinkDeleteNode(LinkNode **head, LinkNode *node) {
    LinkNode *cur = *head;
    if (cur == NULL) return NULL;
    if (cur == node) {
        *head = cur->next;
        free(cur);
        return *head;
    }
    while (cur->next != NULL) {
        if (cur->next == node) {
            LinkNode *next = cur->next;
            cur->next = next->next;
            free(next);
            return cur->next;
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

void LinkReleaseAndFree(LinkNode **head, void (*free_data)(void*)) {
    LinkNode *cur = *head;
    while (cur != NULL) {
        LinkNode *next = cur->next;
        free_data(cur->data);
        free(cur);
        cur = next;
    }
    *head = NULL;
}

void* LinkPop(LinkNode **head) {
    if (*head == NULL) {
        IndexError("LinkPop", "The list is empty");
        return NULL;
    }
    LinkNode *cur = *head;
    if (cur->next == NULL) {
        void *data = cur->data;
        free(cur);
        *head = NULL;
        return data;
    }
    while (cur->next->next != NULL) {
        cur = cur->next;
    }
    void *data = cur->next->data;
    free(cur->next);
    cur->next = NULL;
    return data;
}

void* LinkPopFirst(LinkNode **head) {
    if (*head == NULL) {
        IndexError("LinkPopFirst", "The list is empty");
        return NULL;
    }
    LinkNode *cur = *head;
    void *data = cur->data;
    *head = cur->next;
    free(cur);
    return data;
}