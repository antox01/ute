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
    //int scrolly;
    int screen_width, screen_height;
    int curr_buffer;
    string_builder_t sb; // Buffer to use when saving the file on the disk
    Buffer buffer;
    //Buffers buffers;
    char cwd[MAX_STR_SIZE];
    string_builder_t command_output;
} Editor;


void sb_append(string_builder_t *sb, const char *str, size_t str_len);
void sb_append_char(string_builder_t *sb, const char val);

int read_file(Buffer *buffer, string_builder_t *sb);
int write_file();
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

    update_display(&ute);
    /* assert(0 && "TODO: unhandled move"); */
    /* move(buffer->cy, buffer->cx); */

    int cx, cy;
    buffer_cyx(buffer, &cy, &cx);
    move(cy, cx);
    while (!stop) {
        ch = getch();
	
        stop = manage_key(&ute, ch);
	//stop = 1;
        buffer = current_buffer(&ute);
        update_display(&ute);
	buffer_cyx(buffer, &cy, &cx);
	move(cy, cx);
    
        /* move(buffer->cy, buffer->cx); */
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
/*
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
    if(ch == 127) {
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
    char *str = buffer_str(buffer);
    move(0,0);
    // TODO: make the update_display able to scroll
    // TODO: try to parse the buffer per line, to have a better management of the scrolls
    clear();
    addstr(str);
    free(str);
    //TODO: update display managing the reset of the cursor
//    assert(0 && "TODO: update_display not implemented");
    /* Buffer_ *buffer = current_buffer(&ute); */
    /* char str[MAX_STR_SIZE] = {0}; */
    /* clear(); */
    /* for(int i = 0; i + buffer->scrolly < buffer->lines.count && i < buffer->height && i < ute.screen_height; i++) { */
    /*     move(i, 0); */
    /*     int line_pos = i + buffer->scrolly; */
    /*     int num_display_char = buffer->lines.data[line_pos].count < buffer->width ? */
    /*         buffer->lines.data[line_pos].count : buffer->width; */
    /*     num_display_char = num_display_char < ute.screen_width ? num_display_char : ute.screen_width; */
    /*     if(num_display_char > 0) { */
    /*         memcpy(str, buffer->lines.data[line_pos].data, num_display_char); */
    /*         //buffer[num_display_char] = '\0'; */
    /*         //addstr(buffer); */
    /*         addnstr(str, num_display_char); */
    /*     } */
    /* } */

    print_status_line(ute);
    print_command_line(ute);
    refresh();

}

void print_status_line(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    int sline_pos = ute->screen_height - STATUS_LINE_SPACE;
    int sline_right_start = ute->screen_width - STATUS_LINE_RIGHT_CHAR;
    char str[MAX_STR_SIZE] = {0};
    int left_len;
    left_len = snprintf(str, MAX_STR_SIZE, "%s", "[New File]");
    memset(&str[left_len], ' ', sline_right_start - left_len);
    int right_len = snprintf(&str[sline_right_start], MAX_STR_SIZE - sline_right_start,
            "%d,%d", buffer->cy + buffer->scrolly + 1, buffer->cx + 1);
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

int read_file(Buffer *buffer, string_builder_t *sb) {
//    assert(0 && "TODO: read_file not implemented");
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


    buffer_insert_str(buffer, sb->data, sb->count);
defer:
    free(sb->data);
    fclose(fin);
    return ret;
}

int write_file() {
    assert(0 && "TODO: write_file not implemented");
/*     Buffer_ *buffer = current_buffer(&ute); */
/*     int ret = 0, str_size; */
/*     char str[MAX_STR_SIZE]; */
/*     ute.sb.count = 0; */
/*     for(int i = 0; i < buffer->lines.count; i++) { */
/*         sb_append(&ute.sb, buffer->lines.data[i].data, buffer->lines.data[i].count); */
/*         sb_append_char(&ute.sb, '\n'); */
/*     } */

/*     if(buffer->file_name == NULL) { */
/*         buffer->file_name = read_command_line("File name: "); */
/*     } */

/*     FILE *fout = fopen(buffer->file_name, "w"); */
/*     if(fout == NULL) ret_defer(1); */
/*     fwrite(ute.sb.data, sizeof(*ute.sb.data), ute.sb.count, fout); */
/*     if(ferror(fout)) ret_defer(1); */

/*     str_size = snprintf(str, MAX_STR_SIZE, */
/*             "\"%s\" written %ld bytes", buffer->file_name, ute.sb.count * sizeof(*ute.sb.data)); */
/*     ute.command_output.count = 0; */
/*     sb_append(&ute.command_output, str, str_size); */

/* defer: */
/*     if(fout != NULL) fclose(fout); */
/*     return ret; */
}

int open_file(Editor *ute, char *file_name) {
    int ret = 1;
    Buffer buffer = buffer_init(0);

    assert(file_name != NULL && "ERROR: open_file does not support NULL");

    buffer.file_name = file_name;

    ret = read_file(&buffer, &ute->sb);
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
    assert(0 && "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer + 1) % ute->buffers.count;
}

void buffers_prev(Editor *ute) {
    assert(0 && "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer - 1 + ute->buffers.count) % ute->buffers.count;
}
