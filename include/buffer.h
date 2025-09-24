#ifndef BUFFER_H_DEF
#define BUFFER_H_DEF

#include "line.h"
#include "common.h"

#define BUF_CAPACITY 1024

typedef struct buffer_s_ {
    char *data;
    int capacity;
    int cursor;
    int gap_end;
    int dirty;
    char *file_name;

    Lines lines;
} Buffer;



ute_da(Buffer, Buffers);


/* Buffer buffer_init(int width, int height); */
/* void buffer_free(Buffer buffer); */

/* void buffer_set_size(Buffer *buffer, int width, int height); */
void buffer_free(Buffer *gb);


void buffer_prev_line(Buffer *buffer);
void buffer_next_line(Buffer *buffer);
/* void buffer_forward(Buffer *buffer); */
/* void buffer_backward(Buffer *buffer); */
void buffer_forward_word(Buffer *buffer);
void buffer_backward_word(Buffer *buffer);
void buffer_right(Buffer *gb);
void buffer_left(Buffer *gb);
void buffer_remove(Buffer *gb);
void buffer_insert(Buffer *gb, char c);
void buffer_insert_str(Buffer *gb, char *str, int size);
void buffer_set_cursor(Buffer *gb, int cursor);
/* void buffer_add_char_cl(Buffer_ *buffer, char ch); */
int buffer_size(Buffer *gb);
char* buffer_str(Buffer *gb);

void buffer_posyx(Buffer *gb, int pos, int *cy, int *cx);
void buffer_parse_line(Buffer *gb);
void buffer_search_word(Buffer *gb, char *word, size_t word_len);

#endif//BUFFER_H_DEF
