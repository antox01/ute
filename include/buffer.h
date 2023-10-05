#ifndef BUFFER_H_DEF
#define BUFFER_H_DEF

#include "line.h"

typedef struct buffer_s {
    int cx, cy;
    int scrolly;
    int width, height;
    Lines lines;
    char *file_name;
    struct buffer_s *next;
    struct buffer_s *prev;
} Buffer;

Buffer buffer_init(int width, int height);
void buffer_free(Buffer buffer);

void buffer_prev_line(Buffer *buffer);
void buffer_next_line(Buffer *buffer);
void buffer_forward(Buffer *buffer);
void buffer_backward(Buffer *buffer);

#endif//BUFFER_H_DEF
