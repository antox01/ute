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

#define is_printable(x) (0x20 <= x && x <= 0xFF)

ute_da(char, string_builder_t)


typedef struct {
    //int cx, cy;
    //int scrolly;
    int screen_width, screen_height;
    int curr_buffer;
    string_builder_t sb; // Buffer to use when saving the file on the disk
    //Buffer buffer;
    Buffers buffers;
    char cwd[MAX_STR_SIZE];
    string_builder_t command_output;
} Editor;
Editor ute = {0};

void sb_append(string_builder_t *sb, const char *str, size_t str_len);
void sb_append_char(string_builder_t *sb, const char val);

int read_file(Buffer *buffer, string_builder_t *sb);
int write_file();
int open_file(Editor *ute, char *file_name);
int get_file_size(FILE *fin, size_t *size);
int manage_key(int ch);
void update_display();
void print_status_line();
void print_command_line();
char *read_command_line(const char* msg);

void delete_char(Editor *ute);
void new_line(Editor *ute);

Buffer *current_buffer(Editor *ute);
void buffers_next(Editor *ute);
void buffers_prev(Editor *ute);

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    shift_args(&argc, &argv);

    getcwd(ute.cwd, MAX_STR_SIZE);

    //printf("%s\n", ute.cwd);
    //return 0;

    //ute.buffer = buffer_init();

    //return 0;

    //printf("%d\n", KEY_DC);
    //return 0;

    initscr();
    keypad(stdscr, 1);
    raw();
    noecho();
    getmaxyx(stdscr, ute.screen_height, ute.screen_width);

    if(argc > 0) {
        open_file(&ute, strdup(shift_args(&argc, &argv)));
    } else {
        ute_da_append(&ute.buffers,
                buffer_init(ute.screen_width, ute.screen_height - STATUS_LINE_SPACE));
    }

    Buffer *buffer = current_buffer(&ute);

    //buffer->width = ute.screen_width;
    //buffer->height = ute.screen_height - STATUS_LINE_SPACE;

    int ch, stop = 0;

    //ch = getch();
    //int ch2;
    //ch2 = getch();
    ////ch3 = getch();

    //endwin();
    //printf("%d\n", ch);
    //printf("%d\n", KEY_CTRL('f'));
    //printf("%d\n", ch2);
    //printf("%d\n", ch3);
    //return 0;

    update_display();
    move(buffer->cy, buffer->cx);
    while (!stop) {
        ch = getch();

        stop = manage_key(ch);

        buffer = current_buffer(&ute);
        update_display();
        move(buffer->cy, buffer->cx);
    }
    endwin();
    //printf("%d\n", KEY_BACKSPACE);
    //printf("%d\n", ch_save);
    return 0;
}

int manage_key(int ch) {
    Buffer *buffer = current_buffer(&ute);
    if(ch == KEY_CTRL('c'))
        return 1;

    if(ch == KEY_CTRL('s')) {
        write_file();
        buffer->saved = 1;
    }

    if(ch == KEY_CTRL('f')) {
        buffer_forward_word(buffer);
    }
    if(ch == KEY_CTRL('b')) {
        buffer_backward_word(buffer);
    }
    if(ch == KEY_CTRL('o')) {
        open_file(&ute, NULL);
    }
    if(ch == KEY_CTRL('p')) {
        buffers_prev(&ute);
    }
    if(ch == KEY_CTRL('n')) {
        buffers_next(&ute);
    }

    if(ch == 10) {
        new_line(&ute);
        buffer->saved = 0;
    } 

    if(KEY_CTRL('a') <= ch && ch <= KEY_CTRL('z')) {
        //TODO: manage all ctrl keybinding
        return 0;
    }

    if(ch == KEY_RESIZE) {
        getmaxyx(stdscr, ute.screen_height, ute.screen_width);
        buffer_set_size(buffer, ute.screen_width, ute.screen_height - STATUS_LINE_SPACE);
    }

    switch (ch) {
        case KEY_DOWN:
            buffer_next_line(buffer);
            break;
        case KEY_UP:
            buffer_prev_line(buffer);
            break;
        case KEY_RIGHT:
            buffer_forward(buffer);
            break;
        case KEY_LEFT:
            buffer_backward(buffer);
            break;
    }
    if(ch == 127) {
        delete_char(&ute);
        buffer->saved = 0;
    } else if(ch == KEY_DC){
        int line_pos = buffer->cy + buffer->scrolly;
        if(buffer->cx < buffer->lines.data[line_pos].count) {
            buffer->cx++;
            delete_char(&ute);
        } else if (line_pos < buffer->lines.count-1){
            buffer->cx = 0;
            buffer->cy++;
            delete_char(&ute);
        }
        buffer->saved = 0;
    } else if(is_printable(ch)) {
        if(buffer->lines.max_size <= 0) {
            ute_da_append(&buffer->lines, line_init());
        }

        // Convert tab key to multiple spaces
        if(ch == '\t') {
            for(int i = 0; i < TAB_TO_SPACE; i++) {
                //line_add_char(&ute.lines[ute.cy], ' ', ute.cx++);
                buffer_add_char_cl(buffer, ' ');
            }
        } else {
            //line_add_char(&ute.lines[ute.cy], ch, ute.cx);
            //ute.cx++;
            buffer_add_char_cl(buffer, ch);
        }
        buffer->saved = 0;
    }
    return 0;
}

void update_display() {
    Buffer *buffer = current_buffer(&ute);
    char str[MAX_STR_SIZE] = {0};
    clear();
    for(int i = 0; i + buffer->scrolly < buffer->lines.count && i < buffer->height && i < ute.screen_height; i++) {
        move(i, 0);
        int line_pos = i + buffer->scrolly;
        int num_display_char = buffer->lines.data[line_pos].count < buffer->width ?
            buffer->lines.data[line_pos].count : buffer->width;
        num_display_char = num_display_char < ute.screen_width ? num_display_char : ute.screen_width;
        if(num_display_char > 0) {
            memcpy(str, buffer->lines.data[line_pos].data, num_display_char);
            //buffer[num_display_char] = '\0';
            //addstr(buffer);
            addnstr(str, num_display_char);
        }
    }

    print_status_line();
    print_command_line();
    //refresh();
}

void print_status_line() {
    Buffer *buffer = current_buffer(&ute);
    int sline_pos = ute.screen_height - STATUS_LINE_SPACE;
    int sline_right_start = ute.screen_width - STATUS_LINE_RIGHT_CHAR;
    char str[MAX_STR_SIZE] = {0};
    int left_len;
    if (buffer->file_name == NULL) {
        left_len = snprintf(str, MAX_STR_SIZE, "%s", "[New File]");
    } else {
        left_len =  snprintf(str, MAX_STR_SIZE, "%s", buffer->file_name);
        if (!buffer->saved) {
            left_len += snprintf(&str[left_len], MAX_STR_SIZE - left_len, " [+]");
        }
    }
    memset(&str[left_len], ' ', sline_right_start - left_len);
    int right_len = snprintf(&str[sline_right_start], MAX_STR_SIZE - sline_right_start,
            "%d,%d", buffer->cy + buffer->scrolly + 1, buffer->cx + 1);
    memset(&str[sline_right_start + right_len], ' ', ute.screen_width - right_len - sline_right_start);
    str[ute.screen_width] = '\0';
    attron(A_REVERSE);
    move(sline_pos, 0);
    addstr(str);
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
    while (ch != '\n' && ch != KEY_CTRL('c')) {
        if(ch == 127) {
            if(ute.command_output.count > start) {
                ute.command_output.count -= 1;
                ute.command_output.data[ute.command_output.count] ='\0';
            }
        } else {
            sb_append_char(&ute.command_output, ch);
        }
        print_command_line();
        ch = getch();
    }
    if(ch != KEY_CTRL('c')) {
        ret = strdup(&ute.command_output.data[start]);
    } else {
        ute.command_output.count = 0;
    }
    return ret;
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file(Buffer *buffer, string_builder_t *sb) {
    size_t file_size;
    int ret = 1;

    if(sb->max_size > 0) {
        free(sb->data);
        sb->count = 0;
        sb->max_size = 0;
    }

    FILE *fin = fopen(buffer->file_name, "r");

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    sb->data = malloc(file_size * sizeof(*sb->data));
    sb->max_size = file_size;
    fread(sb->data, sizeof(*sb->data), file_size, fin);
    sb->count = file_size;

    int cur_row = 0;
    for(int i = 0; i < file_size; i++) {
        if(sb->data[i] == '\n') {
            Line line = line_init();
            line_append(&line, &sb->data[cur_row], i - cur_row);
            line.data[i-cur_row] = '\0';
            ute_da_append(&buffer->lines, line);
            cur_row = i+1;
        }
    }

defer:
    //free(sb.data);
    fclose(fin);
    return ret;
}

int write_file() {
    Buffer *buffer = current_buffer(&ute);
    int ret = 0, str_size;
    char str[MAX_STR_SIZE];
    ute.sb.count = 0;
    for(int i = 0; i < buffer->lines.count; i++) {
        sb_append(&ute.sb, buffer->lines.data[i].data, buffer->lines.data[i].count);
        sb_append_char(&ute.sb, '\n');
    }

    if(buffer->file_name == NULL) {
        buffer->file_name = read_command_line("File name: ");
    }

    FILE *fout = fopen(buffer->file_name, "w");
    if(fout == NULL) ret_defer(1);
    fwrite(ute.sb.data, sizeof(*ute.sb.data), ute.sb.count, fout);
    if(ferror(fout)) ret_defer(1);

    str_size = snprintf(str, MAX_STR_SIZE,
            "\"%s\" written %ld bytes", buffer->file_name, ute.sb.count * sizeof(*ute.sb.data));
    ute.command_output.count = 0;
    sb_append(&ute.command_output, str, str_size);

defer:
    if(fout != NULL) fclose(fout);
    return ret;
}

int open_file(Editor *ute, char *file_name) {
    int ret = 1;
    Buffer buffer = buffer_init(ute->screen_width, ute->screen_height - STATUS_LINE_SPACE);
    if(file_name == NULL) {
        buffer.file_name = read_command_line("Open file: ");
    } else {
        buffer.file_name = file_name;
    }
    if (buffer.file_name == NULL) {
        return 0;
    }
    ret = read_file(&buffer, &ute->sb);
    if (ret) {
        ute_da_append(&ute->buffers, buffer);
        ute->curr_buffer = ute->buffers.count - 1;
    }
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
    Buffer *buffer = current_buffer(ute);
    int line_pos = buffer->cy + buffer->scrolly;
    if(buffer->cx > 0) {
        line_del_char(&buffer->lines.data[line_pos], buffer->cx-1);
        buffer->cx--;
    } else if (line_pos > 0) {
        int old_prev_line_count = buffer->lines.data[line_pos - 1].count;
        line_append_line(&buffer->lines.data[line_pos-1], buffer->lines.data[buffer->cy]);
        line_free(buffer->lines.data[line_pos]);
        if(line_pos < buffer->lines.count) {
            memmove(&buffer->lines.data[line_pos], &buffer->lines.data[line_pos+1],
                    sizeof(*buffer->lines.data)*(buffer->lines.count - line_pos));
        }
        buffer->lines.count--;
        buffer->cy--;
        buffer->cx = old_prev_line_count;
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
    Buffer *buffer = current_buffer(ute);
    Line line = line_init();
    int addcy = 1;
    if(buffer->cx <= 0) {
        buffer->cy--;
        addcy++;
    } else if(buffer->cx < buffer->lines.data[buffer->cy].count){
        line_append(&line, &buffer->lines.data[buffer->cy].data[buffer->cx], buffer->lines.data[buffer->cy].count - buffer->cx);
        buffer->lines.data[buffer->cy].data[buffer->cx] = '\0';
        buffer->lines.data[buffer->cy].count -= line.count;
    }

    if(buffer->lines.max_size <= 0) {
        ute_da_append(&buffer->lines, line);
    } else if(buffer->lines.count >= buffer->lines.max_size) {
        buffer->lines.data = realloc(buffer->lines.data, sizeof(*buffer->lines.data)*(buffer->lines.max_size+1));
        if(buffer->cy < buffer->lines.count) {
            memmove(&buffer->lines.data[buffer->cy+2], &buffer->lines.data[buffer->cy + 1],
                    sizeof(*buffer->lines.data)*(buffer->lines.count - buffer->cy -1));
        }
        buffer->lines.count++;
        buffer->lines.max_size++;
        buffer->lines.data[buffer->cy + 1] = line;
    } else {
        if(buffer->cy < buffer->lines.count) {
            memmove(&buffer->lines.data[buffer->cy+2], &buffer->lines.data[buffer->cy + 1],
                    sizeof(*buffer->lines.data)*(buffer->lines.count - buffer->cy -1));
        }
        buffer->lines.count++;
        buffer->lines.data[buffer->cy + 1] = line;
    }
    buffer->cy+=addcy;
    buffer->cx = 0;
}

Buffer *current_buffer(Editor *ute) {
    if (ute->buffers.count == 0) {
        return NULL;
    }
    return &ute->buffers.data[ute->curr_buffer];
}

void buffers_next(Editor *ute) {
    ute->curr_buffer = (ute->curr_buffer + 1) % ute->buffers.count;
}

void buffers_prev(Editor *ute) {
    ute->curr_buffer = (ute->curr_buffer - 1 + ute->buffers.count) % ute->buffers.count;
}
