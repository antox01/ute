#ifndef HISTORY_H
#define HISTORY_H

#include "utils.h"

typedef enum {
    CMD_NONE,
    CMD_DELETE,
    CMD_INSERT,
} Command_Kind;

typedef struct {
    Command_Kind kind;
    String_Builder sb;
    int cursor_start;
    int cursor_end;
} Command;

typedef struct {
    Command *data;
    size_t count;
    size_t max_size;
} Commands;

typedef struct {
    Commands undo_list;
    Commands redo_list;

    Command current;
} History;

void history_insert_char(History *h, int cursor, char c);
void history_delete_char(History *h, int cursor, char c);
bool history_undo(History *h, void *b);
bool history_redo(History *h, void *b);
void history_free(History *h);

#endif // HISTORY_H
