#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"

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

void buffer_insert_str(Buffer *gb, char* str, int str_size) {
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

void buffer_next_line(Buffer *gb) {
    int cl_pos = gb->cursor;
    int nl_pos = gb->gap_end;
    int nl_virt_pos = gb->cursor+1;
    while(cl_pos >= 0 && gb->data[cl_pos] != '\n' && gb->data[cl_pos] != '\r') cl_pos--;
    while(nl_virt_pos < buffer_size(gb)
	&& gb->data[nl_pos] != '\n' && gb->data[nl_pos] != '\r'){
	nl_pos++;
	nl_virt_pos++;
    }
    int limit = nl_pos + 1;
    nl_virt_pos++;
    while(nl_virt_pos < buffer_size(gb)
	&& gb->data[limit] != '\n' &&  gb->data[limit] != '\r'){
	limit++;
	nl_virt_pos++;
    }
    
    // aaa  aaaa
    //    ^
    // aaaaaaa
    int nc = gb->cursor - cl_pos + nl_pos;
    if(nc > gb->capacity) {
	// TODO: add error codes when messages support is implemented
	return;
    }
    if(nc > limit) {
	nc = limit;
    }
    nc -= gb->gap_end - gb->cursor;
    buffer_set_cursor(gb, nc);
//    assert(0 && "TODO: Not implemented");
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

void buffer_cyx(Buffer *gb, int *cy, int *cx) {
    int line = 0, start = 0;
    for(int i = 0; i < gb->cursor; i++) {
	if(gb->data[i] == '\n') {
	    line++;
	    start = i+1;
	}
    }

    *cy = line;
    *cx = gb->cursor - start;
}
