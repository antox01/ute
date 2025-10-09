#ifndef HISTORY_H
#define HISTORY_H

#include "utils.h"

typedef enum {
    CMD_DELETE,
    CMD_INSERT,
} Command_Kind;

typedef struct {
    String_Builder sb;
    int cursor_start;
    int cursor_end;
} Delete;

typedef struct {

} Insert;

typedef struct {
    Delete del;
    Command_Kind kind;
} Command;

typedef struct {
    Command *data;
    size_t count;
    size_t max_size;
} Commands;

typedef struct {
    Commands undo_list;
    Commands redo_list;
} History;

void history_free(History *h);

#endif // HISTORY_H
