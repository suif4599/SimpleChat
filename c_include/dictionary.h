struct Dict
{
    size_t max_len;
    size_t len;
    int *keys;
    void **values;
};

struct Dict* Dict_Create(size_t size);
void Dict_Delete(struct Dict dict);
int Dict_Add(struct Dict dict, int key, void *value);
int Dict_Remove(struct Dict dict, int key);
int Dict_Modify(struct Dict dict, int key, void *value);