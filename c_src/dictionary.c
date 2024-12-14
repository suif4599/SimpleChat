#include <stdlib.h>

#include "dictionary.h"
#include "error_interface.h"

struct Dict* Dict_Create(size_t size)
{
    struct Dict* dict = malloc(sizeof(struct Dict));
    if (dict == NULL)
    {
        show_error("MemoryError", "Could not allocate memory for dictionary.");
        return NULL;
    }
    dict->keys = (int*)malloc(size);
    if (dict->keys == NULL)
    {
        show_error("MemoryError", "Could not allocate memory for dictionary keys.");
        return NULL;
    }
    dict->values = (void**)malloc(size);
    if (dict->values == NULL)
    {
        show_error("MemoryError", "Could not allocate memory for dictionary values.");
        return NULL;
    }
    dict->max_len = size;
    dict->len = 0;
    return dict;
}

void Dict_Destroy(struct Dict dict)
{
    free(dict.keys);
    free(dict.values);
}

int Dict_Add(struct Dict dict, int key, void *value)
{
    if (dict.len == dict.max_len) return -1;
    int index = find_gt(dict.keys, dict.len, key);
    if (index == dict.len)
    {
        dict.keys[dict.len] = key;
        dict.values[dict.len] = value;
        dict.len++;
        return 0;
    }
    for (size_t i = dict.len; i > index; i--)
    {
        dict.keys[i] = dict.keys[i - 1];
        dict.values[i] = dict.values[i - 1];
    }
    dict.keys[index] = key;
    dict.values[index] = value;
    dict.len++;
    return 0;
}

int Dict_Remove(struct Dict dict, int key)
{
    int index = find_gt(dict.keys, dict.len, key);
    if (index == dict.len) return -1;
    for (size_t i = index; i < dict.len - 1; i++)
    {
        dict.keys[i] = dict.keys[i + 1];
        dict.values[i] = dict.values[i + 1];
    }
    dict.len--;
    return 0;
}

int Dict_Modify(struct Dict dict, int key, void *value)
{
    int index = find_gt(dict.keys, dict.len, key);
    if (index == 0) return -1;
    if (dict.keys[index - 1] != key) return -1;
    dict.values[index - 1] = value;
    return 0;
}

size_t find_gt(int* list, size_t len, int value) // list is ascending
{
    for (size_t i = 0; i < len; i++)
        if (list[i] > value) return i;
    return len;
}