#include "line.h"
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define MAX_STR_SIZE 256

#define ret_defer(x) do { ret = x; goto defer; } while(0)

struct {
    int cx, cy;
    int scrolly;
    int screen_width, screen_height;
    Line *lines;
    int line_count;
    int max_line_count;
    char *file_name;
    char cwd[MAX_STR_SIZE];
} ute = {0};

typedef struct {
    char *str;
    size_t length;
    size_t max_length;
} string_builder_t;

int read_file();
int get_file_size(FILE *fin, size_t *size);
void update_display();

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    shift_args(&argc, &argv);

    getcwd(ute.cwd, MAX_STR_SIZE);

    //printf("%s\n", ute.cwd);
    //return 0;

    if(argc > 0) {
        ute.file_name = shift_args(&argc, &argv);
    }

    read_file();
    //return 0;

    initscr();
    keypad(stdscr, 1);
    noecho();
    getmaxyx(stdscr, ute.screen_height, ute.screen_width);

    int ch;
    update_display();
    move(ute.cy, ute.cx);
    while (1) {
        ch = getch();

        if(ch == 'q') {
            break;
        }

        switch (ch) {
            case KEY_DOWN:
                if(ute.cy < ute.screen_height && ute.cy < ute.line_count-1) {
                    ute.cy++;
                    if(ute.cx > ute.lines[ute.cy].count) {
                        ute.cx = ute.lines[ute.cy].count;
                    }
                } else if(ute.scrolly + ute.cy < ute.line_count) {
                    ute.scrolly++;
                }
                break;
            case KEY_UP:
                if(ute.cy > 0) {
                    ute.cy--;
                    if(ute.cx > ute.lines[ute.cy].count) {
                        ute.cx = ute.lines[ute.cy].count;
                    }
                } else if(ute.scrolly > 0) {
                    ute.scrolly--;
                }
                break;
            case KEY_RIGHT:
                if(ute.cx < ute.screen_width) {
                    if(ute.line_count > ute.cy && ute.cx < ute.lines[ute.cy].count) {
                        ute.cx++;
                    }
                }
                break;
            case KEY_LEFT:
                if(ute.cx > 0) {
                    ute.cx--;
                }
                break;
        }
        if(ch == 127) {
            if(ute.cx > 0) {
                line_del_char(&ute.lines[ute.cy], ute.cx-1);
                ute.cx--;
            } else if (ute.cy > 0) {
                int old_prev_line_count = ute.lines[ute.cy-1].count;
                line_append_line(&ute.lines[ute.cy-1], ute.lines[ute.cy]);
                line_free(ute.lines[ute.cy]);
                if(ute.cy < ute.line_count) {
                    memcpy(&ute.lines[ute.cy], &ute.lines[ute.cy+1],
                            sizeof(*ute.lines)*(ute.line_count - ute.cy));
                }
                ute.line_count--;
                ute.cy--;
                ute.cx = old_prev_line_count;
            }
        } else if(ch == 10) {
            if(ute.max_line_count <= 0) {
                ute.lines = malloc(sizeof(*ute.lines));
                ute.lines[0] = line_init();
                ute.line_count = ute.max_line_count = 1;
            } else if(ute.line_count >= ute.max_line_count) {
                ute.lines = realloc(ute.lines, sizeof(*ute.lines)*(ute.max_line_count+1));
                if(ute.cy < ute.line_count) {
                    memcpy(&ute.lines[ute.cy+2], &ute.lines[ute.cy + 1],
                            sizeof(*ute.lines)*(ute.line_count - ute.cy -1));
                }
                ute.line_count++;
                ute.max_line_count++;
                ute.lines[ute.cy + 1] = line_init();
            } else {
                ute.line_count++;
            }
            ute.cy++;
            ute.cx = 0;
        } else if(isalpha(ch)) {
            if(ute.max_line_count <= 0) {
                ute.lines = malloc(sizeof(*ute.lines));
                ute.lines[0] = line_init();
                ute.line_count = ute.max_line_count = 1;
            } else if(ute.line_count <= ute.cy) {
                ute.lines = realloc(ute.lines, sizeof(*ute.lines)*(ute.max_line_count+1));
                ute.lines[ute.line_count] = line_init();
                ute.line_count++;
                ute.max_line_count++;
            }

            line_add_char(&ute.lines[ute.cy], ch, ute.cx);
            ute.cx++;
        }
        update_display();
        move(ute.cy, ute.cx);
    }
    endwin();
    //printf("%d\n", KEY_BACKSPACE);
    //printf("%d\n", ch_save);
    return 0;
}

void update_display() {
    char buffer[256] = {0};
    clear();
    for(int i = 0; i + ute.scrolly < ute.line_count && i < ute.screen_height; i++) {
        move(i, 0);
        memset(buffer, 0, 256);
        int line_pos = i + ute.scrolly;
        int num_display_char = ute.lines[line_pos].count < ute.screen_width ?
            ute.lines[line_pos].count : ute.screen_width;
        if(num_display_char > 0) {
            memcpy(buffer, ute.lines[line_pos].data, num_display_char);
            //buffer[num_display_char] = '\0';
            //addstr(buffer);
            addnstr(buffer, num_display_char);
        }
    }

    //refresh();
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file() {
    string_builder_t sb = {0};
    size_t file_size, line_number;
    int ret = 1;

    FILE *fin = fopen(ute.file_name, "r");

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    sb.str = malloc(file_size * sizeof(*sb.str));
    sb.max_length = file_size;
    fread(sb.str, sizeof(*sb.str), file_size, fin);

    // set line_number to 1 because there is at least a line in the file
    line_number = 1;

    for(int i = 0; i < file_size; i++) {
        if(sb.str[i] == '\n') line_number++;
    }

    if(ute.max_line_count <= 0) {
        ute.lines = malloc(sizeof(*ute.lines) * line_number);
        ute.max_line_count = line_number;
    }

    int cur_row = 0, count_line = 0;
    for(int i = 0; i < file_size; i++) {
        if(sb.str[i] == '\n') {
            ute.lines[count_line] = line_init();
            line_append(&ute.lines[count_line], &sb.str[cur_row], i - cur_row);
            ute.lines[count_line].data[i-cur_row] = '\0';
            ute.line_count++;
            cur_row = i+1;
            count_line++;
        }
    }

defer:
    free(sb.str);
    fclose(fin);
    return ret;
}

int get_file_size(FILE *fin, size_t *size) {
    size_t start, end;
    start = ftell(fin);
    if(fseek(fin, 0, SEEK_END) < 0) return 0;
    end = ftell(fin);
    if(fseek(fin, start, SEEK_SET) < 0) return 0;
    *size = end;

    return 1;
}
