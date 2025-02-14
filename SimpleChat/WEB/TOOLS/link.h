#ifndef __LINK_H__
#define __LINK_H__

#include "../../../ERROR/error.h"
#include <stdlib.h>

typedef struct LinkNodeType{
    void *data;
    struct LinkNodeType *next;
} LinkNode;


int LinkAppend(LinkNode **head, void *data); // Append a new node to the end of the list
int LinkDeleteData(LinkNode **head, void *data); // Delete a node from the list, it won't free data, return -1 if not found, it won't raise any error
LinkNode* LinkDeleteNode(LinkNode **head, LinkNode *node); // Delete a node from the list, it won't free data, returns the next node
void LinkRelease(LinkNode **head); // Release the list, it won't release the data, it won't raise any error
void LinkReleaseAndFree(LinkNode **head, void (*free_data)(void*)); // Release the list, it will free the data, it won't raise any error
void* LinkPop(LinkNode **head); // Pop the last node from the list, it won't free the node, return NULL if the list is empty
void* LinkPopFirst(LinkNode **head); // Get the data of the node at index, return NULL if the list is empty

#endif