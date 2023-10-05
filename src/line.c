#include <string.h>

#include "line.h"

#define LINE_MIN_SIZE 16

Line line_init() {
    Line line = {0};
    line.data = malloc(LINE_MIN_SIZE * sizeof(*line.data));
    line.data[0] = '\0';
    line.count = 0;
    line.max_size = LINE_MIN_SIZE;
    return line;
}

void line_free(Line line) {
    if(line.data != NULL) {
        free(line.data);
    }
}

void line_append(Line *line, const char *str, size_t str_count) {
    if(line->count + str_count >= line->max_size) {
        size_t max_size = (line->count + str_count) / LINE_MIN_SIZE + 1;
        max_size *= LINE_MIN_SIZE;
        line->data = realloc(line->data, max_size * sizeof(char));
        
        if(line->data == NULL) {
            return;
        }
        line->max_size = max_size;
    }

    memcpy(&line->data[line->count], str, str_count + 1);
    line->count += str_count;
}

void line_add_char(Line *line, char ch, int pos) {
    if(line->count + 1 >= line->max_size) {
        size_t max_size = line->max_size * 2;
        line->data = realloc(line->data, max_size * sizeof(char));
        
        if(line->data == NULL) {
            return;
        }
        line->max_size = max_size;
    }

    memcpy(&line->data[pos+1], &line->data[pos], (line->count - pos + 1) * sizeof(char));
    line->data[pos] = ch;
    line->count++;
}

void line_append_line(Line *dst, Line src) {
    line_append(dst, src.data, src.count);
}

void line_del_char(Line *line, int pos) {
    memcpy(&line->data[pos], &line->data[pos+1], line->count - pos);
    line->count--;
}

Lines lines_init() {
    Lines lines = {0};
    return lines;
}

void lines_free(Lines lines) {
    if (lines.max_size > 0) {
        free(lines.data);
    }
}
