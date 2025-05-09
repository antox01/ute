#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include "buffer.h"
#include "common.h"

#define MAX_STR_SIZE 256
#define STATUS_LINE_SPACE 2
#define STATUS_LINE_RIGHT_CHAR 20
#define TAB_TO_SPACE 4

#define KEY_CTRL(x) (x & 0x1F)

#define is_printable(x) (0x20 <= x && x <= 0xFF)

ute_da(char, string_builder_t)


typedef struct {
    //int cx, cy;
    int scroll;
    int screen_width, screen_height;
    int curr_buffer;
    Buffer buffer;
    //Buffers buffers;
    char cwd[MAX_STR_SIZE];
    string_builder_t command_output;
} Editor;


void sb_append(string_builder_t *sb, const char *str, size_t str_len);
void sb_append_char(string_builder_t *sb, const char val);

int read_file(Buffer *buffer);
int write_file(Editor *ute);
int open_file(Editor *ute, char *file_name);
int get_file_size(FILE *fin, size_t *size);
int manage_key(Editor *ute, int ch);
void update_display(Editor *ute);
void print_status_line(Editor *ute);
void print_command_line(Editor *ute);
char *read_command_line(Editor *ute, const char* msg);

void delete_char(Editor *ute);

Buffer *current_buffer(Editor *ute);
void buffers_next(Editor *ute);
void buffers_prev(Editor *ute);

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    Editor ute = {0};
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
        /* ute_da_append(&ute.buffers, buffer_init(0)); */
        ute.buffer = buffer_init(0);
    }

    //Buffer *buffer = current_buffer(&ute);

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

    update_display(&ute);
    /* assert(0 && "TODO: unhandled move"); */
    /* move(buffer->cy, buffer->cx); */

    while (!stop) {
        ch = getch();
	
        stop = manage_key(&ute, ch);
        //buffer = current_buffer(&ute);
        update_display(&ute);
    }
    endwin();
    //printf("%d\n", KEY_BACKSPACE);
    //printf("%d\n", ch_save);
    return 0;
}

int manage_key(Editor *ute, int ch) {
    Buffer *buffer = current_buffer(ute);
    if(ch == KEY_CTRL('c'))
        return 1;
    if(ch == KEY_CTRL('s')) {
        write_file(ute);
        buffer->saved = 1;
    }
/*

    if(ch == KEY_CTRL('f')) {
        buffer_forward_word(buffer);
    }
    if(ch == KEY_CTRL('b')) {
        buffer_backward_word(buffer);
    }
    if(ch == KEY_CTRL('o')) {
	// TODO: support open file
//        open_file(ute, NULL);
    }
    if(ch == KEY_CTRL('p')) {
        buffers_prev(ute);
    }
    if(ch == KEY_CTRL('n')) {
        buffers_next(ute);
    }
*/
    if(ch == 10) {
	buffer_insert(buffer, ch);
        buffer->saved = 0;
    } 

    if(KEY_CTRL('a') <= ch && ch <= KEY_CTRL('z')) {
        //TODO: manage all ctrl keybinding
        return 0;
    }

    if(ch == KEY_RESIZE) {
        getmaxyx(stdscr, ute->screen_height, ute->screen_width);
        //buffer_set_size(buffer, ute.screen_width, ute.screen_height - STATUS_LINE_SPACE);
    }

    switch (ch) {
        case KEY_DOWN:
            buffer_next_line(buffer);
            break;
        case KEY_UP:
            buffer_prev_line(buffer);
            break;
        case KEY_RIGHT:
            buffer_right(buffer);
            break;
        case KEY_LEFT:
            buffer_left(buffer);
            break;
    }
    if(ch == 127 || ch == KEY_BACKSPACE) {
        delete_char(ute);
        buffer->saved = 0;
    } else if(ch == KEY_DC){
	//int line_pos = buffer->cy + buffer->scrolly;
	assert(0 && "TODO: not implemented");
    } else if(is_printable(ch)) {
        // Convert tab key to multiple spaces
        if(ch == '\t') {
            for(int i = 0; i < TAB_TO_SPACE; i++) {
                buffer_insert(buffer, ' ');
            }
        } else {
            //line_add_char(&ute.lines[ute.cy], ch, ute.cx);
            //ute.cx++;
            buffer_insert(buffer, ch);
        }
        buffer->saved = 0;
    }
    return 0;
}

void update_display(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    int save_cursor = buffer->cursor;

    move(0,0);
    // TODO: make the update_display able to scroll horizontally
    // TODO: try to parse the buffer per line, to have a better management of the scrolls
    clear();

    // Check to see if I need to scroll
    int cy, cx;
    int sy, sx;
    buffer_posyx(buffer, buffer->cursor, &cy, &cx);
    buffer_posyx(buffer, ute->scroll, &sy, &sx);

    buffer_set_cursor(buffer, buffer_size(buffer));
    char *str = buffer->data;

    int height = ute->screen_height - STATUS_LINE_SPACE;
    if(cy < sy) {
        ute->scroll -= 2;
        while(ute->scroll > 0 && str[ute->scroll] != '\n') ute->scroll--;
    } else if(height <= cy - sy) {
        while(ute->scroll < buffer->cursor && str[ute->scroll] != '\n') ute->scroll++;
        ute->scroll++;
    }
    
    int cur_char = ute->scroll;
    int cur_x = 0;
    int cur_y = 0;
    while(cur_char < buffer_size(buffer) && cur_y < height) {
        if(cur_x == ute->screen_width - 1) {
            cur_y++;
            cur_x = 0;
            while(cur_char < buffer_size(buffer) && str[cur_char] != '\n') cur_char++;
        } else if(str[cur_char] == '\n') {
            cur_y++;
            cur_x = 0;
        }
        // TODO: Maybe there is a need for a buffer to print everything faster
        // There should not be any problems with printing the character if it is a new line
        addch(str[cur_char]);
        cur_char++;
    }
    
    //TODO: update_display managing the reset of the cursor
    print_status_line(ute);
    print_command_line(ute);
    refresh();

    buffer_set_cursor(buffer, save_cursor);

    buffer_posyx(buffer, ute->scroll, &sy, &sx);
    cur_y = cy - sy;
    if(cur_y >= height) cur_y = height - 1;
    move(cur_y, cx);
}

void print_status_line(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    int sline_pos = ute->screen_height - STATUS_LINE_SPACE;
    int sline_right_start = ute->screen_width - STATUS_LINE_RIGHT_CHAR;
    char str[MAX_STR_SIZE] = {0};
    int left_len;
    int cy, cx;
    buffer_posyx(buffer, buffer->cursor, &cy, &cx);
    if (ute->buffer.file_name) {
        left_len = snprintf(str, MAX_STR_SIZE, "%s", ute->buffer.file_name);
    } else {
        left_len = snprintf(str, MAX_STR_SIZE, "%s", "[New File]");
    }
    memset(&str[left_len], ' ', sline_right_start - left_len);
    int right_len = snprintf(&str[sline_right_start], MAX_STR_SIZE - sline_right_start,
            "%d,%d", cy /* + buffer->scrolly */ + 1, cx + 1);
    memset(&str[sline_right_start + right_len], ' ', ute->screen_width - right_len - sline_right_start);
    str[ute->screen_width] = '\0';
    attron(A_REVERSE);
    move(sline_pos, 0);
    addstr(str);
    attroff(A_REVERSE);
}

void print_command_line(Editor *ute) {
    move(ute->screen_height - 1, 0);
    addnstr(ute->command_output.data, ute->command_output.count);
}

char *read_command_line(Editor *ute, const char* msg) {
    int start = 0;
    char *ret = NULL;
    ute->command_output.count = 0;
    sb_append(&ute->command_output, msg, strlen(msg));
    start = ute->command_output.count;
    print_command_line(ute);
    int ch = getch();
    while (ch != '\n' && ch != KEY_CTRL('c')) {
        if(ch == 127) {
            if(ute->command_output.count > start) {
                ute->command_output.count -= 1;
                ute->command_output.data[ute->command_output.count] ='\0';
            }
        } else {
            sb_append_char(&ute->command_output, ch);
        }
        print_command_line(ute);
        ch = getch();
    }
    if(ch != KEY_CTRL('c')) {
        ret = strdup(&ute->command_output.data[start]);
    } else {
        ute->command_output.count = 0;
    }
    return ret;
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    assert(arg != NULL && "Argomento nullo");
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file(Buffer *buffer) {
//    assert(0 && "TODO: read_file not implemented");
    size_t file_size;
    int ret = 1;

    FILE *fin = fopen(buffer->file_name, "r");
    char str_buf[MAX_STR_SIZE];

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    while(file_size > 0) {
        int n = fread(str_buf, sizeof(*str_buf), MAX_STR_SIZE, fin);
        file_size -= n;
        buffer_insert_str(buffer, str_buf, n);
    }

    buffer_set_cursor(buffer, 0);
defer:
    fclose(fin);
    return ret;
}

int write_file(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    assert(buffer->file_name != NULL && "TODO: write_file does not support command line");

    FILE *fout = fopen(buffer->file_name, "w");
    if(fout == NULL) return 1;

    int saved_cursor = buffer->cursor;
    int buf_size = buffer_size(buffer);

    buffer_set_cursor(buffer, buf_size);

    while(buf_size > 0) {
        int n = fwrite(buffer->data, sizeof(*buffer->data), buf_size, fout);
        assert(n != 0);
        buf_size -= n/sizeof(*buffer->data);
    }

    buffer_set_cursor(buffer, saved_cursor);

    fclose(fout);
    return 0;
}

int open_file(Editor *ute, char *file_name) {
    int ret = 1;
    Buffer buffer = buffer_init(0);

    assert(file_name != NULL && "ERROR: open_file does not support NULL");

    buffer.file_name = file_name;

    ret = read_file(&buffer);
    if (ret) {
        ute->buffer = buffer;
    }
    return ret;
    /* if(file_name == NULL) { */
    /*     buffer.file_name = read_command_line("Open file: "); */
    /* } else { */
    /*     buffer.file_name = file_name; */
    /* } */
    /* if (buffer.file_name == NULL) { */
    /*     return 0; */
    /* } */
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
//    assert(0 && "TODO: delete_char not implemented");
    Buffer *buffer = current_buffer(ute);
    buffer_remove(buffer);
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

Buffer *current_buffer(Editor *ute) {
//    assert(0 && "ERROR: current_buffer not implemented");
    return &ute->buffer;
    /* if (ute->buffers.count == 0) { */
    /*     return NULL; */
    /* } */
    /* return &ute->buffers.data[ute->curr_buffer]; */
}

void buffers_next(Editor *ute) {
    (void) ute;
    assert(0 && "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer + 1) % ute->buffers.count;
}

void buffers_prev(Editor *ute) {
    (void) ute;
    assert(0 && "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer - 1 + ute->buffers.count) % ute->buffers.count;
}


// TODO: Cursor needs to take into account tab characters when moving
// TODO: Parse Buffer by line, to optimize some things
// TODO: Use a Buffer structure for the command line, to have automatic history
// TODO: Improve keybinding management
