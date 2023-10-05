#ifndef BUFFER_H_DEF
#define BUFFER_H_DEF

#include "line.h"
#include "common.h"

typedef struct buffer_s {
    int cx, cy;
    int scrolly;
    int width, height;
    Lines lines;
    char *file_name;
} Buffer;

ute_da(Buffer, Buffers);


Buffer buffer_init();
void buffer_free(Buffer buffer);

void buffer_prev_line(Buffer *buffer);
void buffer_next_line(Buffer *buffer);
void buffer_forward(Buffer *buffer);
void buffer_backward(Buffer *buffer);
void buffer_forward_word(Buffer *buffer);

void buffer_add_char_cl(Buffer *buffer, char ch);

#endif//BUFFER_H_DEF
