#ifndef BUFFER_H_DEF
#define BUFFER_H_DEF

#include "history.h"
#include "line.h"
#include "utils.h"

#define BUF_CAPACITY 1024

typedef struct buffer_s_ {
    char *data;
    int capacity;
    int cursor;
    int gap_end;
    int dirty;
    char *file_name;

    int mark_position;
    int sx;
    int sy;

    History history;
    Lines lines;
    String_Builder sb;
} Buffer;

typedef struct {
    Buffer *data;
    size_t count;
    size_t max_size;
} Buffers;


/* Buffer buffer_init(int width, int height); */
/* void buffer_free(Buffer buffer); */

/* void buffer_set_size(Buffer *buffer, int width, int height); */
void buffer_free(Buffer *gb);
void buffer_reset(Buffer *gb);


void buffer_prev_line(Buffer *buffer);
void buffer_next_line(Buffer *buffer);
void buffer_forward_word(Buffer *buffer);
void buffer_backward_word(Buffer *buffer);
void buffer_right(Buffer *gb);
void buffer_left(Buffer *gb);
void buffer_remove(Buffer *gb);
void buffer_remove_selection(Buffer *gb);
void buffer_insert(Buffer *gb, char c);
void buffer_insert_str(Buffer *gb, char *str, int size);
void buffer_set_cursor(Buffer *gb, int cursor);
int buffer_size(Buffer *gb);
char* buffer_str(Buffer *gb);

void buffer_posyx(Buffer *gb, size_t pos, int *cy, int *cx);
void buffer_parse_line(Buffer *gb);

#endif//BUFFER_H_DEF
