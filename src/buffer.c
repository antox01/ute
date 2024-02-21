#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"
#include "line.h"

Buffer buffer_init(int size) {
    Buffer buf = {0};
    int capacity = max(BUF_CAPACITY, size);
    buf.data = (char *) malloc(sizeof(char) * capacity);
    assert(buf.data != NULL && "Not enough memory");
    buf.cursor = 0;
    buf.gap_end = capacity;
    buf.capacity = capacity;
    return buf;
}

void buffer_free(Buffer *gb) {
    free(gb->data);
}

void buffer_grow(Buffer *gb) {
    int nc = gb->capacity * 2;
    gb->data = (char*) realloc(gb->data, sizeof(char) * nc);
    assert(gb->data != NULL && "Not enough memory");
    if(gb->gap_end < gb->capacity) {
	int rem = gb->capacity - gb->gap_end;
	memmove(&gb->data[nc - rem], &gb->data[gb->cursor], rem);
	gb->gap_end = nc - rem;
    }
    gb->capacity = nc;
}

void buffer_remove(Buffer *gb) {
    if(gb->cursor > 0) gb->cursor--;
}

void buffer_insert(Buffer *gb, char c) {
    if(gb->cursor >= gb->gap_end) buffer_grow(gb);
    gb->data[gb->cursor] = c;
    gb->cursor++;
}

void buffer_insert_str(Buffer *gb, char* str) {
    int str_size = strlen(str);
    for(int i = 0; i < str_size; i++) {
	buffer_insert(gb, str[i]);
    }
}

int buffer_size(Buffer *gb) {
    return gb->capacity - gb->gap_end + gb->cursor;
}

void buffer_right(Buffer *gb) {
    if(gb->gap_end < gb->capacity) gb->data[gb->cursor++] = gb->data[gb->gap_end++];
}

void buffer_left(Buffer *gb) {
    if(0 < gb->cursor) gb->data[--gb->gap_end] = gb->data[--gb->cursor];
}

void buffer_set_cursor(Buffer *gb, int cursor) {
    int diff = cursor - gb-> cursor;
    for(; diff < 0; diff++) {
	buffer_left(gb);
    }
    for(; diff > 0; diff--) {
	buffer_right(gb);
    }
}

char* buffer_str(Buffer *gb) {
    int size = buffer_size(gb);
    char *str = (char*) malloc(sizeof(char) * (size + 1));
    memcpy(str, gb->data, gb->cursor);
    memcpy(&str[gb->cursor], &gb->data[gb->gap_end], size - gb->cursor);
    str[size] = '\0';
    return str;
}

void buffer_prev_line(Buffer *buffer) {
    assert(0 && "TODO: Not implemented!!");
}

void buffer_next_line(Buffer *buffer) {
    assert(0 && "TODO: Not implemented");
}


void buffer_forward_word(Buffer *buffer) {
    assert(0 && "TODO: buffer_forward_word not implemented");
/*     char current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*     while (isalpha(current_char)) { */
/*         buffer_forward(buffer); */
/*         current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*     } */
/* space_skip: */
/*     while (isspace(current_char) || (isprint(current_char) && !isalpha(current_char))) { */
/*         buffer_forward(buffer); */
/*         current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*     } */
/*     if (current_char == '\0') { */
/*         buffer_next_line(buffer); */
/*         buffer->cx = 0; */
/*         current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*         goto space_skip; */
/*     } */
}

void buffer_backward_word(Buffer *buffer) {
    assert(0 && "TODO: buffer_backward_word not implemented");
/*     if (buffer->cy == 0 && buffer->cx == 0) { */
/*         return; */
/*     } */
/*     if (buffer->cx == 0) { */
/*         buffer_prev_line(buffer); */
/*         buffer->cx = buffer->lines.data[buffer->cy].count; */
/*     } */
/*     buffer->cx--; */
/*     char current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*     while (isspace(current_char) || (isprint(current_char) && !isalpha(current_char))) { */
/*         buffer_backward(buffer); */
/*         current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*     } */

/*     while (isalpha(current_char)) { */
/*         buffer_backward(buffer); */
/*         current_char = buffer->lines.data[buffer->cy].data[buffer->cx]; */
/*         if(buffer->cx == 0) { */
/*             buffer->cx--; */
/*             break; */
/*         } */
/*     } */
/*     buffer->cx++; */
}
