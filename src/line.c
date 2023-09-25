#include <stdlib.h>
#include <string.h>

#include "line.h"

#define LINE_MIN_SIZE 16

struct line_s {
    char *data;
    size_t count;
    size_t max_size;
};

Line line_init() {
    Line line = malloc(sizeof(*line));
    line->data = malloc(LINE_MIN_SIZE * sizeof(*line->data));
    line->count = 0;
    line->max_size = LINE_MIN_SIZE;
    return line;
}

void line_free(Line line) {
    if(line->data != NULL) {
        free(line->data);
    }
    free(line);
}

void line_append(Line line, const char *str, size_t str_count) {
    if(line->count + str_count >= line->max_size) {
        size_t max_size = line->max_size * 2;
        line->data = realloc(line->data, max_size * sizeof(char));
        
        if(line->data == NULL) {
            return;
        }
        line->max_size = max_size;
    }

    memcpy(&line->data[line->count], str, str_count + 1);
    line->count += str_count;
}

void line_add_char(Line line, char ch, int pos) {
    if(line->count + 1 >= line->max_size) {
        size_t max_size = line->max_size * 2;
        line->data = realloc(line->data, max_size * sizeof(char));
        
        if(line->data == NULL) {
            return;
        }
        line->max_size = max_size;
    }

    memcpy(&line->data[pos+1], &line->data[pos], line->count - pos);
    line->data[pos] = ch;
    line->count++;
}

void line_append_line(Line dst, Line src) {
    line_append(dst, src->data, src->count);
}
