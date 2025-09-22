#ifndef LINE_H
#define LINE_H

#include <stdlib.h>
#include "common.h"

typedef struct line_s {
    size_t start;
    size_t end;
} Line;

typedef struct {
    Line *data;
    size_t count;
    size_t max_size;
} Lines;


void line_free(Line line);
void line_append(Line *line, const char* str, unsigned long str_count);
void line_append_line(Line *dst, Line src);
void line_add_char(Line *line, char ch, int pos);
void line_del_char(Line *line, int pos);

Lines lines_init();
void lines_free(Lines lines);

#endif //LINE_H
