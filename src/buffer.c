#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"

void buffer_free(Buffer *gb) {
    if(gb->file_name != NULL) free(gb->file_name);
    if(gb->data != NULL) free(gb->data);
    if(gb->lines.data != NULL) free(gb->lines.data);
    if(gb->sb.data != NULL) free(gb->sb.data);
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
    size_t new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;

    buffer_set_cursor(gb, new_cursor);
}

void buffer_next_line(Buffer *gb) {
    int cx, cy;
    buffer_posyx(gb, gb->cursor, &cy, &cx);

    // TODO: report error when cursor on the last line
    if((size_t)cy + 1 >= gb->lines.count) return;

    Line line = gb->lines.data[cy + 1];
    size_t new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;

    buffer_set_cursor(gb, new_cursor);
}


void buffer_forward_word(Buffer *buffer) {
    while(buffer->cursor < buffer_size(buffer)
            && isalnum(buffer->data[buffer->cursor])) buffer_right(buffer);

    while(buffer->cursor < buffer_size(buffer)
            && !isalnum(buffer->data[buffer->cursor])) {
        buffer_right(buffer);
    }
}

void buffer_backward_word(Buffer *buffer) {
    buffer_left(buffer);
    while(buffer->cursor > 0
            && !isalnum(buffer->data[buffer->cursor])) buffer_left(buffer);

    while(buffer->cursor > 0
            && isalnum(buffer->data[buffer->cursor])) buffer_left(buffer);
    buffer_right(buffer);
}

void buffer_posyx(Buffer *gb, size_t pos, int *cy, int *cx) {
    size_t line = 0;
    if(gb->lines.count == 0) {
        *cy = 0;
        *cx = 0;
        return;
    }

    for(; line < gb->lines.count && gb->lines.data[line].end < pos; line++);

    if(line == gb->lines.count) {
        line--;
    }

    *cy = line;
    *cx = pos - gb->lines.data[line].start;
}

void buffer_parse_line(Buffer *gb) {
    int line_start = 0, cur_char = 0;
    int saved_cursor = gb->cursor;
    buffer_set_cursor(gb, buffer_size(gb));
    gb->lines.count = 0;
    gb->sb.count = 0;
    while(cur_char < buffer_size(gb)) {
        if(gb->data[cur_char] == '\n') {
            Line line = {
                .start = line_start,
                .end = cur_char,
            };
            ute_da_append(&gb->lines, line);
            line_start = cur_char+1;
        }
        ute_da_append(&gb->sb, gb->data[cur_char]);
        cur_char++;
    }
    if(line_start <= cur_char) {
        Line line = {
            .start = line_start,
            .end = cur_char,
        };
        ute_da_append(&gb->lines, line);
    }
    buffer_set_cursor(gb, saved_cursor);
}


void buffer_remove_selection(Buffer *gb) {
    if(gb->mark_position > gb->cursor) {
        int tmp = gb->cursor;
        buffer_set_cursor(gb, gb->mark_position);
        gb->mark_position = tmp;
    }
    while(gb->cursor > gb->mark_position) buffer_remove(gb);
}
