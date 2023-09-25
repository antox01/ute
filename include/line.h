#ifndef LINE_H
#define LINE_H

typedef struct line_s *Line;

Line line_init();
void line_free(Line line);
void line_append(Line line, const char* str, unsigned long str_count);
void line_append_line(Line dst, Line src);
void line_add_char(Line line, char ch, int pos);

#endif //LINE_H
