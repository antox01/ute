#include <ctype.h>
#include <stdlib.h>

#include "buffer.h"
#include "line.h"

Buffer buffer_init(int width, int height) {
    Buffer buffer = {0};
    buffer.lines = lines_init();
    buffer.saved = 1;
    buffer.width = width;
    buffer.height = height;
    return buffer;
}

void buffer_free(Buffer buffer) {
    if (buffer.lines.max_size > 0) {
        lines_free(buffer.lines);
    }
    if (buffer.file_name != NULL) {
        free(buffer.file_name);
    }
}


void buffer_prev_line(Buffer *buffer) {
    if(buffer->cy > 0) {
        buffer->cy--;
        int line_pos = buffer->cy + buffer->scrolly;
        if(buffer->cx > buffer->lines.data[line_pos].count) {
            buffer->cx = buffer->lines.data[line_pos].count;
        }
    } else if(buffer->scrolly > 0) {
        buffer->scrolly--;
    }
}

void buffer_next_line(Buffer *buffer) {
    if(buffer->cy < buffer->height - 1 && buffer->cy < buffer->lines.count-1) {
        buffer->cy++;
        int line_pos = buffer->cy + buffer->scrolly;
        if(buffer->cx > buffer->lines.data[line_pos].count) {
            buffer->cx = buffer->lines.data[line_pos].count;
        }
    } else if(buffer->scrolly + buffer->cy < buffer->lines.count - 1) {
        buffer->scrolly++;
    }
}


void buffer_forward(Buffer *buffer) {
    if(buffer->cx < buffer->width) {
        int line_pos = buffer->cy + buffer->scrolly;
        if(buffer->lines.count > line_pos && buffer->cx < buffer->lines.data[line_pos].count) {
            buffer->cx++;
        }
    }
}


void buffer_backward(Buffer *buffer) {
    if(buffer->cx > 0) {
        buffer->cx--;
    }
}

void buffer_forward_word(Buffer *buffer) {
    char current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
    while (isalpha(current_char)) {
        buffer_forward(buffer);
        current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
    }
space_skip:
    while (isspace(current_char)) {
        buffer_forward(buffer);
        current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
    }
    if (current_char == '\0') {
        buffer_next_line(buffer);
        buffer->cx = 0;
        current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
        goto space_skip;
    }
}

void buffer_backward_word(Buffer *buffer) {
    char current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
    if (buffer->cx == 0) {
        buffer_prev_line(buffer);
        buffer->cx = buffer->lines.data[buffer->cy].count;
    }
    buffer->cx--;
    while (isspace(current_char)) {
        buffer_backward(buffer);
        current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
    }

    while (isalpha(current_char)) {
        buffer_backward(buffer);
        current_char = buffer->lines.data[buffer->cy].data[buffer->cx];
        if(buffer->cx == 0) {
            buffer->cx--;
            break;
        }
    }
    buffer->cx++;
}

void buffer_add_char_cl(Buffer *buffer, char ch) {
    line_add_char(&buffer->lines.data[buffer->cy + buffer->scrolly], ch, buffer->cx++);
}
