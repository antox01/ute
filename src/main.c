#include "line.h"
#include <assert.h>
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define MAX_STR_SIZE 256
#define STATUS_LINE_SPACE 2
#define STATUS_LINE_RIGHT_CHAR 20

#define KEY_CTRL(x) (x & 0x1F)

#define ret_defer(x) do { ret = x; goto defer; } while(0)

typedef struct {
    char *str;
    size_t count;
    size_t max_length;
} string_builder_t;


typedef struct {
    int cx, cy;
    int scrolly;
    int screen_width, screen_height;
    string_builder_t sb;
    Line *lines;
    int line_count;
    int max_line_count;
    char *file_name;
    char cwd[MAX_STR_SIZE];
    char command_output[MAX_STR_SIZE];
    int command_size;
} Editor;
Editor ute = {0};

void sb_append(string_builder_t *sb, const char *str, size_t str_len);
void sb_append_char(string_builder_t *sb, const char val);

int read_file();
int write_file();
int get_file_size(FILE *fin, size_t *size);
void update_display();
void print_status_line();
void print_command_line();

void delete_char(Editor *ute);

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    shift_args(&argc, &argv);

    getcwd(ute.cwd, MAX_STR_SIZE);

    //printf("%s\n", ute.cwd);
    //return 0;

    if(argc > 0) {
        ute.file_name = shift_args(&argc, &argv);
        read_file();
    }

    //return 0;

    //printf("%d\n", KEY_DC);
    //return 0;

    initscr();
    keypad(stdscr, 1);
    raw();
    noecho();
    getmaxyx(stdscr, ute.screen_height, ute.screen_width);

    int ch;

    //ch = getch();

    //endwin();
    //printf("%d\n", ch);
    //return 0;

    update_display();
    move(ute.cy, ute.cx);
    while (1) {
        ch = getch();

        if(ch == KEY_CTRL('c'))
            break;

        if(ch == KEY_CTRL('s')) {
            write_file();
        }

        switch (ch) {
            case KEY_DOWN:
                if(ute.cy < ute.screen_height - STATUS_LINE_SPACE - 1 && ute.cy < ute.line_count-1) {
                    ute.cy++;
                    int line_pos = ute.cy + ute.scrolly;
                    if(ute.cx > ute.lines[line_pos].count) {
                        ute.cx = ute.lines[line_pos].count;
                    }
                } else if(ute.scrolly + ute.cy < ute.line_count - 1) {
                    ute.scrolly++;
                }
                break;
            case KEY_UP:
                if(ute.cy > 0) {
                    ute.cy--;
                    int line_pos = ute.cy + ute.scrolly;
                    if(ute.cx > ute.lines[line_pos].count) {
                        ute.cx = ute.lines[line_pos].count;
                    }
                } else if(ute.scrolly > 0) {
                    ute.scrolly--;
                }
                break;
            case KEY_RIGHT:
                if(ute.cx < ute.screen_width) {
                    int line_pos = ute.cy + ute.scrolly;
                    if(ute.line_count > line_pos && ute.cx < ute.lines[line_pos].count) {
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
            delete_char(&ute);
        } else if(ch == KEY_DC){
            int line_pos = ute.cy + ute.scrolly;
            if(ute.cx < ute.lines[line_pos].count) {
                ute.cx++;
                delete_char(&ute);
            } else if (line_pos < ute.line_count-1){
                ute.cx = 0;
                ute.cy++;
                delete_char(&ute);
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
                if(ute.cy < ute.line_count) {
                    memcpy(&ute.lines[ute.cy+2], &ute.lines[ute.cy + 1],
                            sizeof(*ute.lines)*(ute.line_count - ute.cy -1));
                }
                ute.line_count++;
                ute.lines[ute.cy + 1] = line_init();
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
    char buffer[MAX_STR_SIZE] = {0};
    clear();
    for(int i = 0; i + ute.scrolly < ute.line_count && i < ute.screen_height - STATUS_LINE_SPACE; i++) {
        move(i, 0);
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

    print_status_line();
    print_command_line();
    //refresh();
}

void print_status_line() {
    int sline_pos = ute.screen_height - STATUS_LINE_SPACE;
    int sline_right_start = ute.screen_width - STATUS_LINE_RIGHT_CHAR;
    char buffer[MAX_STR_SIZE] = {0};
    int left_len;
    if (ute.file_name == NULL) {
        left_len = snprintf(buffer, MAX_STR_SIZE, "%s", "[New File]");
    } else {
        left_len =  snprintf(buffer, MAX_STR_SIZE, "%s", ute.file_name);
    }
    memset(&buffer[left_len], ' ', sline_right_start - left_len);
    int right_len = snprintf(&buffer[sline_right_start], MAX_STR_SIZE - sline_right_start,
            "%d,%d", ute.cy + ute.scrolly + 1, ute.cx + 1);
    memset(&buffer[sline_right_start + right_len], ' ', ute.screen_width - right_len - sline_right_start);
    buffer[ute.screen_width] = '\0';
    attron(A_REVERSE);
    move(sline_pos, 0);
    addstr(buffer);
    attroff(A_REVERSE);
}

void print_command_line() {
    move(ute.screen_height - 1, 0);
    addnstr(ute.command_output, ute.command_size);
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file() {
    size_t file_size, line_number;
    int ret = 1;

    if(ute.sb.max_length > 0) {
        free(ute.sb.str);
        ute.sb.count = 0;
        ute.sb.max_length = 0;
    }

    FILE *fin = fopen(ute.file_name, "r");

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    ute.sb.str = malloc(file_size * sizeof(*ute.sb.str));
    ute.sb.max_length = file_size;
    fread(ute.sb.str, sizeof(*ute.sb.str), file_size, fin);
    ute.sb.count = file_size;

    // set line_number to 1 because there is at least a line in the file
    line_number = 0;

    for(int i = 0; i < file_size; i++) {
        if(ute.sb.str[i] == '\n') line_number++;
    }

    if(ute.max_line_count <= 0) {
        ute.lines = malloc(sizeof(*ute.lines) * line_number);
        ute.max_line_count = line_number;
    }

    int cur_row = 0, count_line = 0;
    for(int i = 0; i < file_size; i++) {
        if(ute.sb.str[i] == '\n') {
            ute.lines[count_line] = line_init();
            line_append(&ute.lines[count_line], &ute.sb.str[cur_row], i - cur_row);
            ute.lines[count_line].data[i-cur_row] = '\0';
            ute.line_count++;
            cur_row = i+1;
            count_line++;
        }
    }

defer:
    //free(sb.str);
    fclose(fin);
    return ret;
}

int write_file() {
    int ret = 0;
    ute.sb.count = 0;
    for(int i = 0; i < ute.line_count; i++) {
        sb_append(&ute.sb, ute.lines[i].data, ute.lines[i].count);
        sb_append_char(&ute.sb, '\n');
    }

    assert(ute.file_name != NULL);

    FILE *fout = fopen(ute.file_name, "w");
    if(fout == NULL) ret_defer(1);
    fwrite(ute.sb.str, sizeof(*ute.sb.str), ute.sb.count, fout);
    if(ferror(fout)) ret_defer(1);

    ute.command_size = snprintf(ute.command_output, MAX_STR_SIZE,
            "\"%s\" written %ld bytes", ute.file_name, ute.sb.count * sizeof(*ute.sb.str));

defer:
    if(fout != NULL) fclose(fout);
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

void delete_char(Editor *ute) {
    int line_pos = ute->cy + ute->scrolly;
    if(ute->cx > 0) {
        line_del_char(&ute->lines[line_pos], ute->cx-1);
        ute->cx--;
    } else if (line_pos > 0) {
        int old_prev_line_count = ute->lines[line_pos - 1].count;
        line_append_line(&ute->lines[line_pos-1], ute->lines[ute->cy]);
        line_free(ute->lines[line_pos]);
        if(line_pos < ute->line_count) {
            memcpy(&ute->lines[line_pos], &ute->lines[line_pos+1],
                    sizeof(*ute->lines)*(ute->line_count - line_pos));
        }
        ute->line_count--;
        ute->cy--;
        ute->cx = old_prev_line_count;
    }
}

void sb_append(string_builder_t *sb, const char *str, size_t str_len) {
    if(sb->count + str_len >= sb->max_length) {
        size_t new_size = sb->count + str_len + 1;
        sb->str = realloc(sb->str, new_size * sizeof(*sb->str));
        sb->max_length = new_size;
    }
    memcpy(&sb->str[sb->count], str, (str_len + 1) * sizeof(*str));
    sb->count += str_len;
}

void sb_append_char(string_builder_t *sb, const char val) {
    if(sb->count + 1 >= sb->max_length) {
        size_t new_size = sb->count + 2;
        sb->str = realloc(sb->str, new_size * sizeof(*sb->str));
        sb->max_length = new_size;
    }
    //memcpy(&sb->str[sb->count], str, (str_len + 1) * sizeof(*str));
    sb->str[sb->count] = val;
    sb->str[sb->count + 1] = '\0';
    sb->count += 1;
}
