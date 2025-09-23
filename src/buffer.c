#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"

void buffer_free(Buffer *gb) {
    if(gb->file_name != NULL) free(gb->file_name);
    free(gb->data);
}

void buffer_grow(Buffer *gb) {
    int nc = gb->capacity * 2;
    if(nc == 0) nc = BUF_CAPACITY;
    gb->data = (char*) realloc(gb->data, sizeof(char) * nc);
    assert(gb->data != NULL && "Not enough memory");
    if(gb->capacity == 0 || gb->gap_end <= gb->capacity) {
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
    assert(gb->cursor < gb->gap_end && "Impossible");
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

void buffer_prev_line(Buffer *gb) {
    int cx, cy;
    buffer_posyx(gb, gb->cursor, &cy, &cx);

    // TODO: report error when cursor on the last line
    if(cy <= 0) return;

    Line line = gb->lines.data[cy - 1];
    int new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;

    buffer_set_cursor(gb, new_cursor);
}

void buffer_next_line(Buffer *gb) {
    int cx, cy;
    buffer_posyx(gb, gb->cursor, &cy, &cx);

    // TODO: report error when cursor on the last line
    if(cy + 1 >= gb->lines.count) return;

    Line line = gb->lines.data[cy + 1];
    int new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;

    buffer_set_cursor(gb, new_cursor);
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

void buffer_posyx(Buffer *gb, int pos, int *cy, int *cx) {
    int line = 0;
    for(; line < gb->lines.count && gb->lines.data[line].end < pos; line++);

    *cy = line;
    *cx = pos - gb->lines.data[line].start;
}

void buffer_parse_line(Buffer *gb) {
    int line_start = 0, cur_char = 0;
    int saved_cursor = gb->cursor;
    buffer_set_cursor(gb, buffer_size(gb));
    while(cur_char < buffer_size(gb)) {
        if(gb->data[cur_char] == '\n') {
            Line line = {
                .start = line_start,
                .end = cur_char,
            };
            ute_da_append(&gb->lines, line);
            line_start = cur_char+1;
        }
        cur_char++;
    }
    // if(line_start < cur_char) {
    //     Line line = {
    //         .start = line_start,
    //         .end = cur_char,
    //     };
    //     ute_da_append(&gb->lines, line);
    // }
    buffer_set_cursor(gb, saved_cursor);
}
