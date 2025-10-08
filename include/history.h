#ifndef HISTORY_H
#define HISTORY_H

#include "utils.h"

typedef struct {
    String_Builder insert;
    String_Builder remove;
    int insert_start;
    int remove_start;
} History_Item;


typedef struct {
    History_Item *data;
    size_t count;
    size_t max_size;
} History_Items;

typedef struct {
    History_Items undo_list;
    History_Items redo_list;
} History;

void history_free(History *h);
void history_item_free(History_Item *hi);

#endif // HISTORY_H
