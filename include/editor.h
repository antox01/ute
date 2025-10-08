#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include "history.h"

typedef struct {
    int *data;
    size_t count;
    size_t max_size;
} Display_Attributes;

typedef struct {
    bool up_to_date;

    char *data;
    size_t count;
    size_t max_size;

    Display_Attributes attr;

    size_t highlight_search;
    size_t highlight_count;
} Display;

typedef enum {
    NORMAL_MODE = 0,
    INSERT_MODE,
} Editor_Mode;

typedef struct {
    //int cx, cy;
    int scroll; // This variable is used to store the position of the first character to display
    int screen_width, screen_height;
    int curr_buffer;
    Display display;
    Buffers buffers;
    History history;
    char cwd[MAX_STR_SIZE];
    Buffer command;
    Editor_Mode mode;
} Editor;

Buffer *current_buffer(Editor *ute);
void update_display(Editor *ute);
String_View read_command_line(Editor *ute, const char* msg);

void print_status_line(Editor *ute);
void print_command_line(Editor *ute, const char* msg);


int editor_search_word(Editor *ute);
int editor_open(Editor *ute);
int editor_write(Editor *ute);
int editor_command(Editor *ute);
int editor_buffers_next(Editor *ute);
int editor_buffers_prev(Editor *ute);
#endif// EDITOR_H
