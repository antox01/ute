#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "buffer.h"
#include "common.h"
#include "line.h"

#define MAX_STR_SIZE 256
#define STATUS_LINE_SPACE 2
#define STATUS_LINE_RIGHT_CHAR 20
#define TAB_TO_SPACE 4

#define KEY_CTRL(x) (x & 0x1F)

ute_da(char, string_builder_t)


typedef struct {
    //int cx, cy;
    //int scrolly;
    int screen_width, screen_height;
    string_builder_t sb; // Buffer to use when saving the file on the disk
    Buffer buffer;
    char cwd[MAX_STR_SIZE];
    string_builder_t command_output;
} Editor;
Editor ute = {0};

void sb_append(string_builder_t *sb, const char *str, size_t str_len);
void sb_append_char(string_builder_t *sb, const char val);

int read_file();
int write_file();
int get_file_size(FILE *fin, size_t *size);
int manage_key(int ch);
void update_display();
void print_status_line();
void print_command_line();
char *read_command_line(const char* msg);

void delete_char(Editor *ute);
void new_line(Editor *ute);

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    shift_args(&argc, &argv);

    getcwd(ute.cwd, MAX_STR_SIZE);

    //printf("%s\n", ute.cwd);
    //return 0;

    ute.buffer = buffer_init();
    if(argc > 0) {
        ute.buffer.file_name = strdup(shift_args(&argc, &argv));
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

    ute.buffer.width = ute.screen_width;
    ute.buffer.height = ute.screen_height - STATUS_LINE_SPACE;

    int ch, stop = 0;

    //ch = getch();
    //int ch2;
    //ch2 = getch();
    ////ch3 = getch();

    //endwin();
    //printf("%d\n", ch);
    //printf("%d\n", ch2);
    ////printf("%d\n", ch3);
    //return 0;

    update_display();
    move(ute.buffer.cy, ute.buffer.cx);
    while (!stop) {
        ch = getch();

        stop = manage_key(ch);

        update_display();
        move(ute.buffer.cy, ute.buffer.cx);
    }
    endwin();
    //printf("%d\n", KEY_BACKSPACE);
    //printf("%d\n", ch_save);
    return 0;
}

int manage_key(int ch) {
    if(ch == KEY_CTRL('c'))
        return 1;

    if(ch == KEY_CTRL('s')) {
        write_file();
    }

    if(ch == KEY_RESIZE) {
        getmaxyx(stdscr, ute.screen_height, ute.screen_width);
    }

    switch (ch) {
        case KEY_DOWN:
            //if(ute.cy < ute.screen_height - STATUS_LINE_SPACE - 1 && ute.cy < ute.line_count-1) {
            //    ute.cy++;
            //    int line_pos = ute.cy + ute.scrolly;
            //    if(ute.cx > ute.lines[line_pos].count) {
            //        ute.cx = ute.lines[line_pos].count;
            //    }
            //} else if(ute.scrolly + ute.cy < ute.line_count - 1) {
            //    ute.scrolly++;
            //}
            buffer_next_line(&ute.buffer);
            break;
        case KEY_UP:
            //if(ute.cy > 0) {
            //    ute.cy--;
            //    int line_pos = ute.cy + ute.scrolly;
            //    if(ute.cx > ute.lines[line_pos].count) {
            //        ute.cx = ute.lines[line_pos].count;
            //    }
            //} else if(ute.scrolly > 0) {
            //    ute.scrolly--;
            //}
            buffer_prev_line(&ute.buffer);
            break;
        case KEY_RIGHT:
            //if(ute.cx < ute.screen_width) {
            //    int line_pos = ute.cy + ute.scrolly;
            //    if(ute.line_count > line_pos && ute.cx < ute.lines[line_pos].count) {
            //        ute.cx++;
            //    }
            //}
            buffer_forward(&ute.buffer);
            break;
        case KEY_LEFT:
            //if(ute.cx > 0) {
            //    ute.cx--;
            //}
            buffer_backward(&ute.buffer);
            break;
    }
    if(ch == 127) {
        delete_char(&ute);
    } else if(ch == KEY_DC){
        int line_pos = ute.buffer.cy + ute.buffer.scrolly;
        if(ute.buffer.cx < ute.buffer.lines.data[line_pos].count) {
            ute.buffer.cx++;
            delete_char(&ute);
        } else if (line_pos < ute.buffer.lines.count-1){
            ute.buffer.cx = 0;
            ute.buffer.cy++;
            delete_char(&ute);
        }
    } else if(ch == 10) {
        new_line(&ute);
    } else if(isalpha(ch) || isdigit(ch) || isspace(ch)) {
        if(ute.buffer.lines.max_size <= 0) {
            //ute.lines = malloc(sizeof(*ute.lines));
            //ute.lines[0] = line_init();
            //ute.line_count = ute.buffer.lines.max_size = 1;
            ute_da_append(&ute.buffer.lines, line_init());
        //} else if(ute.line_count <= ute.cy) {
        //    ute.lines = realloc(ute.lines, sizeof(*ute.lines)*(ute.buffer.lines.max_size+1));
        //    ute.lines[ute.line_count] = line_init();
        //    ute.line_count++;
        //    ute.buffer.lines.max_size++;
        }

        // Convert tab key to multiple spaces
        if(ch == '\t') {
            for(int i = 0; i < TAB_TO_SPACE; i++) {
                //line_add_char(&ute.lines[ute.cy], ' ', ute.cx++);
                buffer_add_char_cl(&ute.buffer, ' ');
            }
        } else {
            //line_add_char(&ute.lines[ute.cy], ch, ute.cx);
            //ute.cx++;
            buffer_add_char_cl(&ute.buffer, ch);
        }
    }
    return 0;
}

void update_display() {
    char buffer[MAX_STR_SIZE] = {0};
    clear();
    for(int i = 0; i + ute.buffer.scrolly < ute.buffer.lines.count && i < ute.screen_height - STATUS_LINE_SPACE; i++) {
        move(i, 0);
        int line_pos = i + ute.buffer.scrolly;
        int num_display_char = ute.buffer.lines.data[line_pos].count < ute.buffer.width ?
            ute.buffer.lines.data[line_pos].count : ute.buffer.width;
        if(num_display_char > 0) {
            memcpy(buffer, ute.buffer.lines.data[line_pos].data, num_display_char);
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
    if (ute.buffer.file_name == NULL) {
        left_len = snprintf(buffer, MAX_STR_SIZE, "%s", "[New File]");
    } else {
        left_len =  snprintf(buffer, MAX_STR_SIZE, "%s", ute.buffer.file_name);
    }
    memset(&buffer[left_len], ' ', sline_right_start - left_len);
    int right_len = snprintf(&buffer[sline_right_start], MAX_STR_SIZE - sline_right_start,
            "%d,%d", ute.buffer.cy + ute.buffer.scrolly + 1, ute.buffer.cx + 1);
    memset(&buffer[sline_right_start + right_len], ' ', ute.screen_width - right_len - sline_right_start);
    buffer[ute.screen_width] = '\0';
    attron(A_REVERSE);
    move(sline_pos, 0);
    addstr(buffer);
    attroff(A_REVERSE);
}

void print_command_line() {
    move(ute.screen_height - 1, 0);
    addnstr(ute.command_output.data, ute.command_output.count);
}

char *read_command_line(const char* msg) {
    int start = 0;
    char *ret = NULL;
    ute.command_output.count = 0;
    sb_append(&ute.command_output, msg, strlen(msg));
    start = ute.command_output.count;
    print_command_line();
    int ch = getch();
    while (ch != '\n') {
        sb_append_char(&ute.command_output, ch);
        print_command_line();
        ch = getch();
    }
    ret = strdup(&ute.command_output.data[start]);
    return ret;
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file() {
    size_t file_size;
    int ret = 1;

    if(ute.sb.max_size > 0) {
        free(ute.sb.data);
        ute.sb.count = 0;
        ute.sb.max_size = 0;
    }

    FILE *fin = fopen(ute.buffer.file_name, "r");

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    ute.sb.data = malloc(file_size * sizeof(*ute.sb.data));
    ute.sb.max_size = file_size;
    fread(ute.sb.data, sizeof(*ute.sb.data), file_size, fin);
    ute.sb.count = file_size;

    int cur_row = 0;
    for(int i = 0; i < file_size; i++) {
        if(ute.sb.data[i] == '\n') {
            Line line = line_init();
            line_append(&line, &ute.sb.data[cur_row], i - cur_row);
            line.data[i-cur_row] = '\0';
            ute_da_append(&ute.buffer.lines, line);
            cur_row = i+1;
        }
    }

defer:
    //free(sb.data);
    fclose(fin);
    return ret;
}

int write_file() {
    int ret = 0, buffer_size;
    char buffer[MAX_STR_SIZE];
    ute.sb.count = 0;
    for(int i = 0; i < ute.buffer.lines.count; i++) {
        sb_append(&ute.sb, ute.buffer.lines.data[i].data, ute.buffer.lines.data[i].count);
        sb_append_char(&ute.sb, '\n');
    }

    if(ute.buffer.file_name == NULL) {
        ute.buffer.file_name = read_command_line("File name: ");
    }

    FILE *fout = fopen(ute.buffer.file_name, "w");
    if(fout == NULL) ret_defer(1);
    fwrite(ute.sb.data, sizeof(*ute.sb.data), ute.sb.count, fout);
    if(ferror(fout)) ret_defer(1);

    buffer_size = snprintf(buffer, MAX_STR_SIZE,
            "\"%s\" written %ld bytes", ute.buffer.file_name, ute.sb.count * sizeof(*ute.sb.data));
    ute.command_output.count = 0;
    sb_append(&ute.command_output, buffer, buffer_size);

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
    int line_pos = ute->buffer.cy + ute->buffer.scrolly;
    if(ute->buffer.cx > 0) {
        line_del_char(&ute->buffer.lines.data[line_pos], ute->buffer.cx-1);
        ute->buffer.cx--;
    } else if (line_pos > 0) {
        int old_prev_line_count = ute->buffer.lines.data[line_pos - 1].count;
        line_append_line(&ute->buffer.lines.data[line_pos-1], ute->buffer.lines.data[ute->buffer.cy]);
        line_free(ute->buffer.lines.data[line_pos]);
        if(line_pos < ute->buffer.lines.count) {
            memcpy(&ute->buffer.lines.data[line_pos], &ute->buffer.lines.data[line_pos+1],
                    sizeof(*ute->buffer.lines.data)*(ute->buffer.lines.count - line_pos));
        }
        ute->buffer.lines.count--;
        ute->buffer.cy--;
        ute->buffer.cx = old_prev_line_count;
    }
}

void sb_append(string_builder_t *sb, const char *str, size_t str_len) {
    if(sb->count + str_len >= sb->max_size) {
        size_t new_size = sb->count + str_len + 1;
        sb->data = realloc(sb->data, new_size * sizeof(*sb->data));
        sb->max_size = new_size;
    }
    memcpy(&sb->data[sb->count], str, (str_len + 1) * sizeof(*str));
    sb->count += str_len;
}

void sb_append_char(string_builder_t *sb, const char val) {
    if(sb->count + 1 >= sb->max_size) {
        size_t new_size = sb->count + 2;
        sb->data = realloc(sb->data, new_size * sizeof(*sb->data));
        sb->max_size = new_size;
    }
    //memcpy(&sb->data[sb->count], str, (str_len + 1) * sizeof(*str));
    sb->data[sb->count] = val;
    sb->data[sb->count + 1] = '\0';
    sb->count += 1;
}

void new_line(Editor *ute) {
    Line line = line_init();
    int addcy = 1;
    if(ute->buffer.cx <= 0) {
        ute->buffer.cy--;
        addcy++;
    } else if(ute->buffer.cx < ute->buffer.lines.data[ute->buffer.cy].count){
        line_append(&line, &ute->buffer.lines.data[ute->buffer.cy].data[ute->buffer.cx], ute->buffer.lines.data[ute->buffer.cy].count - ute->buffer.cx);
        ute->buffer.lines.data[ute->buffer.cy].data[ute->buffer.cx] = '\0';
        ute->buffer.lines.data[ute->buffer.cy].count -= line.count;
    }

    if(ute->buffer.lines.max_size <= 0) {
        ute_da_append(&ute->buffer.lines, line);
    } else if(ute->buffer.lines.count >= ute->buffer.lines.max_size) {
        ute->buffer.lines.data = realloc(ute->buffer.lines.data, sizeof(*ute->buffer.lines.data)*(ute->buffer.lines.max_size+1));
        if(ute->buffer.cy < ute->buffer.lines.count) {
            memcpy(&ute->buffer.lines.data[ute->buffer.cy+2], &ute->buffer.lines.data[ute->buffer.cy + 1],
                    sizeof(*ute->buffer.lines.data)*(ute->buffer.lines.count - ute->buffer.cy -1));
        }
        ute->buffer.lines.count++;
        ute->buffer.lines.max_size++;
        ute->buffer.lines.data[ute->buffer.cy + 1] = line;
    } else {
        if(ute->buffer.cy < ute->buffer.lines.count) {
            memcpy(&ute->buffer.lines.data[ute->buffer.cy+2], &ute->buffer.lines.data[ute->buffer.cy + 1],
                    sizeof(*ute->buffer.lines.data)*(ute->buffer.lines.count - ute->buffer.cy -1));
        }
        ute->buffer.lines.count++;
        ute->buffer.lines.data[ute->buffer.cy + 1] = line;
    }
    ute->buffer.cy+=addcy;
    ute->buffer.cx = 0;
}
