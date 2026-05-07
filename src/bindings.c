#include "buffer.h"
#include "bindings.h"
#include "editor.h"
#include "history.h"
#include "utils.h"

Range motion_forward_word(Editor *ute) {
    Range range = {0};
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer_find_next_word(buffer);
    return range;
}

Range motion_backward_word(Editor *ute) {
    Range range = {0};
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer_find_prev_word(buffer);
    return range;
}

Range motion_left(Editor *ute) {
    Range range = {0};
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer->cursor-1;
    return range;
}

Range motion_right(Editor *ute) {
    Range range = {0};
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer->cursor+1;
    return range;
}

Range motion_prev_line(Editor *ute) {
    int cx, cy;
    Range range;
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer->cursor;

    buffer_posyx(buffer, buffer->cursor, &cy, &cx);

    // TODO: report error when cursor on the last line
    if(cy <= 0) return range;

    Line line = buffer->lines.data[cy - 1];
    size_t new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;
    range.end = new_cursor;

    return range;
}

Range motion_next_line(Editor *ute) {
    int cx, cy;
    Range range;
    Buffer *buffer = current_buffer(ute);
    range.start = buffer->cursor;
    range.end = buffer->cursor;

    buffer_posyx(buffer, buffer->cursor, &cy, &cx);

    // TODO: report error when cursor on the last line
    if((size_t)cy + 1 >= buffer->lines.count) return range;

    Line line = buffer->lines.data[cy + 1];
    size_t new_cursor = line.start + cx;
    if(new_cursor > line.end) new_cursor = line.end;

    range.end = new_cursor;
    return range;
}

Motion_Func *bindings_get_motion(char ch) {
    switch(ch) {
        case 'k': return motion_prev_line;
        case 'j': return motion_next_line;
        case 'h': return motion_left;
        case 'l': return motion_right;
        case 'w': return motion_forward_word;
        case 'b': return motion_backward_word;
        default: return NULL;
    }
    UTE_ASSERT(0, "Unreachable");
}

void operator_delete(Editor *ute, Range range) {
    Buffer *buffer = current_buffer(ute);
    if(range.end < range.start) {
        int tmp = range.end;
        range.end = range.start;
        range.start = tmp;
    }
    history_delete_string(&buffer->history, range.end, &buffer->sb.data[range.start], range.end - range.start);
    buffer_remove_range(buffer, range);
    buffer->dirty = 1;
    ute->display.up_to_date = false;
}

Operator_Func *bindings_get_operator(char ch) {
    switch(ch) {
        case 'd': return operator_delete;
        default: return NULL;
    }
    UTE_ASSERT(0, "Unreachable");
}
