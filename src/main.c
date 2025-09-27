#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "buffer.h"
#include "common.h"
#include "lexer.h"

#define MAX_STR_SIZE 256
#define STATUS_LINE_SPACE 2
#define STATUS_LINE_RIGHT_CHAR 20
#define TAB_TO_SPACE 4
#define EXPAND_TAB false
#define KEY_CTRL(x) (x & 0x1F)
#define KEY_ESCAPE 0x1B

#define DEFAULT_COLOR   1
#define KEYWORD_COLOR   2
#define TYPE_COLOR      3
#define COMMENT_COLOR   4
#define PREPROC_COLOR   5
#define STRING_COLOR    6
#define HIGHLIGHT_COLOR 9

#define is_printable(x) ((0x20 <= x && x <= 0xFF) || x == '\n' || x == '\t')

const char *c_types[] = { "int", "float", "double", "char", "void", "size_t", };

const char *c_keywords[] = {
    "const", "if", "else", "for", "while", "do", "return", "switch", "case", "default",
    "typedef", "struct", "break", "continue",
};

typedef struct {
    char *data;
    size_t count;
    size_t max_size;
} String_Builder;

typedef struct {
    char *data;
    size_t count;
} String_View;

char *sv_to_cstr(String_View sv);


typedef struct {
    NCURSES_COLOR_T *data;
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

    // start of the scroll
    int sx;
    int sy;

    // Real cursor position
    size_t cx;
    size_t cy;
} Display;

typedef struct {
    //int cx, cy;
    int scroll; // This variable is used to store the position of the first character to display
    int screen_width, screen_height;
    int curr_buffer;
    Buffer buffer;
    Display display;
    //Buffers buffers;
    char cwd[MAX_STR_SIZE];
    Buffer command;
} Editor;


// File management functions
int read_file(Buffer *buffer, String_View cwd);
int open_file(Editor *ute, char *file_name);
int get_file_size(FILE *fin, size_t *size);

int manage_key(Editor *ute, int ch);
void update_display(Editor *ute);
void print_status_line(Editor *ute);
void print_command_line(Editor *ute, const char* msg);
String_View read_command_line(Editor *ute, const char* msg);

Buffer *current_buffer(Editor *ute);

void buffers_next(Editor *ute);
void buffers_prev(Editor *ute);

// TODO: make this functions commands of the editor
// to be called when it is possible to input commands
// in the command buffer
void ute_search_word(Editor *ute);
int ute_open(Editor *ute);
int ute_write(Editor *ute);

char *shift_args(int *argc, char ***argv);

int main(int argc, char **argv) {
    Editor ute = {0};
    shift_args(&argc, &argv);

    getcwd(ute.cwd, MAX_STR_SIZE);

    initscr();
    keypad(stdscr, 1);
    raw();
    noecho();
    start_color();

    init_pair(DEFAULT_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(KEYWORD_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(TYPE_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(COMMENT_COLOR, COLOR_CYAN, COLOR_BLACK);
    init_pair(PREPROC_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(STRING_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(HIGHLIGHT_COLOR, COLOR_BLACK, COLOR_YELLOW);

    getmaxyx(stdscr, ute.screen_height, ute.screen_width);
#if 0
    int ch, stop = 0;
    ch = getch();
    endwin();

    printf("%d\n", ch);
    return 0;
#else

    if(argc > 0) {
        open_file(&ute, strdup(shift_args(&argc, &argv)));
    }

    int ch, stop = 0;

    update_display(&ute);

    while (!stop) {
        ch = getch();
	
        stop = manage_key(&ute, ch);
        update_display(&ute);
    }
    endwin();

    buffer_free(&ute.buffer);
    buffer_free(&ute.command);
    free(ute.display.data);
    free(ute.display.attr.data);
    return 0;
#endif
}

int manage_key(Editor *ute, int ch) {
    Buffer *buffer = current_buffer(ute);

    switch (ch) {
        case KEY_CTRL('c'):
            return 1;
        case KEY_CTRL('s'):
            ute_write(ute);
            break;
        case KEY_CTRL('o'):
            // TODO: print error when is not possible to open the file
            ute_open(ute);
            break;
        case KEY_CTRL('f'):
        {
            ute_search_word(ute);
            break;
        }
        case KEY_CTRL('b'):
            break;
        case KEY_RESIZE:
            getmaxyx(stdscr, ute->screen_height, ute->screen_width);
            break;
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
        case KEY_DC:
            buffer_right(buffer);
            fallthrough;
        case 127:
        case KEY_BACKSPACE:
            buffer_remove(buffer);
            buffer->dirty = 1;
            ute->display.up_to_date = false;
            break;

        case KEY_ESCAPE: {
            timeout(0);
            int c = getch();
            if(c != ERR) {
                switch(c) {
                    case 'f':
                        buffer_forward_word(buffer);
                        break;
                    case 'b':
                        buffer_backward_word(buffer);
                        break;
                    default:
                        break;
                }
            }
            timeout(-1);
        } break;
        default: {
            //TODO: manage all ctrl keybinding

            if(is_printable(ch)) {
                // Convert tab key to multiple spaces
                if(EXPAND_TAB && ch == '\t') {
                    for(int i = 0; i < TAB_TO_SPACE; i++) {
                        buffer_insert(buffer, ' ');
                    }
                } else {
                    //line_add_char(&ute.lines[ute.cy], ch, ute.cx);
                    //ute.cx++;
                    buffer_insert(buffer, ch);
                }
                buffer->dirty = 1;
                ute->display.up_to_date = false;
            }
        }
    }
    return 0;
}

void update_display(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    int saved_cursor = buffer->cursor;

    Display *display = &ute->display;

    buffer_set_cursor(buffer, buffer_size(buffer));

    if (!display->up_to_date) {
        display->attr.count = 0;

        buffer_parse_line(buffer);
        UTE_ASSERT(buffer->lines.count > 0, "ERROR: buffer->lines.count cannot be 0");
        UTE_ASSERT(buffer->lines.max_size > 0, "ERROR: buffer->lines.max_size cannot be 0");

        // Reset attribute to be the default one
        for (size_t j = 0; j < (size_t) buffer_size(buffer); j++) ute_da_append(&display->attr, DEFAULT_COLOR);

        Lexer l = lexer_init(buffer->data, buffer_size(buffer));
        while(l.cursor < l.size) {
            lexer_next(&l);
            NCURSES_COLOR_T color = DEFAULT_COLOR;
            switch(l.token.kind) {
                case TOKEN_TYPES:
                    color = TYPE_COLOR;
                    break;
                case TOKEN_KEYWORD:
                    color = KEYWORD_COLOR;
                    break;
                case TOKEN_COMMENT:
                    color = COMMENT_COLOR;
                    break;
                case TOKEN_LITERAL:
                    color = STRING_COLOR;
                    break;
                case TOKEN_PREPROC:
                    color = PREPROC_COLOR;
                    break;
                default:
                    color = DEFAULT_COLOR;
            }
            size_t j = 0;
            while(j < l.token.count) {
                display->attr.data[j + l.token.start] = color;
                j++;
            }
        }

        // Highlight the searched character
        size_t hl_it = 0;
        while(hl_it < ute->display.highlight_count) {
            display->attr.data[hl_it + ute->display.highlight_search] = HIGHLIGHT_COLOR;
            hl_it++;
        }
        display->up_to_date = true;
    }

    // Check to see if I need to scroll
    int cy, cx;
    buffer_posyx(buffer, saved_cursor, &cy, &cx);

    UTE_ASSERT(cx >= 0 && cy >= 0, "ERROR: got cx or cy negative");

    int width = ute->screen_width;
    int height = ute->screen_height - STATUS_LINE_SPACE;

    if(cy < display->sy) display->sy = cy;
    else if(height <= cy - display->sy) display->sy = cy - height + 1;

    if(cx < display->sx) display->sx = cx;
    if(width <= cx - display->sx) display->sx = cx - width + 1;

    int cur_x = 0;
    int cur_y = 0;

    display->count = 0;

    NCURSES_COLOR_T active_attribute = DEFAULT_COLOR;
    attrset(COLOR_PAIR(active_attribute));
    size_t i = 0;
    while(i < (size_t) height && i + display->sy < buffer->lines.count) {
        Line line = buffer->lines.data[i+display->sy];
        move(i, 0);
        size_t curr_char = line.start + display->sx;
        int j = 0;
        while(j < width && curr_char < line.end) {
            NCURSES_COLOR_T new_attribute = display->attr.data[curr_char];
            if(new_attribute != active_attribute) {
                active_attribute = new_attribute;
                addnstr(display->data, display->count);
                display->count = 0;
                attrset(COLOR_PAIR(new_attribute));
            }
            if(buffer->data[curr_char] == '\t') {
                for(int ntab = 0; ntab < TAB_TO_SPACE; ntab++)
                    ute_da_append(display, ' ');
            } else ute_da_append(display, buffer->data[curr_char]);

            j++;
            curr_char++;
        }
        // NOTE: manually cleaning the screen
        // This solved the problem of the editor feeling too slow
        // when displaying stuff on the screen
        while(j++ < width) ute_da_append(display, ' ');

        if(display->count > 0) {
            addnstr(display->data, display->count);
            display->count = 0;
        }
        i++;
    }
    // NOTE: clearing the remaining part of the screen
    // if the text does not occupy it fully
    while(i < (size_t) height) {
        display->count = 0;
        move(i, 0);
        for(int j = 0; j < width; j++) ute_da_append(display, ' ');
        addnstr(display->data, display->count);
        i++;
    }

    // NOTE: take into account characters of different sizes
    for(int curr_char = buffer->lines.data[cy].start; curr_char < saved_cursor; curr_char++) {
        if(buffer->data[curr_char] == '\t') cx += TAB_TO_SPACE - 1;
    }

    buffer_set_cursor(buffer, saved_cursor);
    attroff(COLOR_PAIR(active_attribute));

    //TODO: update_display managing the reset of the cursor
    print_status_line(ute);
    print_command_line(ute, "");
    refresh();

    cur_y = cy - display->sy;
    cur_x = cx - display->sx;
    if(cur_y >= height) cur_y = height - 1;
    move(cur_y, cur_x);
    refresh();
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
    if(buffer->dirty && left_len < MAX_STR_SIZE - 1)
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", " [+]");

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

void print_command_line(Editor *ute, const char* msg) {
    int start = ute->command.cursor;
    Display *display = &ute->display;
    move(ute->screen_height - 1, 0);
    if(ute->command.capacity > 0) {
        while(start >= 0 && ute->command.data[start] != '\n') start--;
        start++;
    }
    UTE_ASSERT(start >= 0, "Impossible");
    ute->display.count = 0;
    int msg_len = strlen(msg);
    if(msg != NULL &&  msg_len > 0) ute_da_append_many(display, msg, msg_len);
    ute_da_append_many(display, &ute->command.data[start], ((int)ute->command.cursor - start));
    int command_cx = display->count;

    while(display->count < (size_t) ute->screen_width) ute_da_append(display, ' ');
    addnstr(display->data, ute->display.count);
    move(ute->screen_height - 1, command_cx);
}

String_View read_command_line(Editor *ute, const char* msg) {
    int start = 0;
    String_View ret = {0};
    start = ute->command.cursor;
    print_command_line(ute, msg);
    int ch = getch();
    while (ch != '\n' && ch != KEY_CTRL('c')) {
        if(ch == 127 || ch == KEY_BACKSPACE) {
            buffer_remove(&ute->command);
        } else {
            buffer_insert(&ute->command, ch);
        }
        print_command_line(ute, msg);
        ch = getch();
    }
    if(ch != KEY_CTRL('c')) {
        ret = (String_View){
            .data = &ute->command.data[start],
            .count = ute->command.cursor - start,
        };
        buffer_insert(&ute->command, '\n');
    } else {
        ute->command.cursor = start;
    }
    return ret;
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    UTE_ASSERT(*argc > 0 && arg != NULL, "ERROR: no arguments provided");
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

int read_file(Buffer *buffer, String_View cwd) {
    size_t file_size;
    int ret = 1;
    size_t filename_lenght = strlen(buffer->file_name);

    String_Builder sb = {0};
    ute_da_append_many(&sb, cwd.data, cwd.count);
    ute_da_append(&sb, '/');
    ute_da_append_many(&sb, buffer->file_name, filename_lenght);
    ute_da_append(&sb, 0);

    FILE *fin = fopen(sb.data, "r");
    char str_buf[MAX_STR_SIZE];

    if(fin == NULL) ret_defer(0);

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    while(file_size > 0) {
        int n = fread(str_buf, sizeof(*str_buf), MAX_STR_SIZE, fin);
        file_size -= n;
        buffer_insert_str(buffer, str_buf, n);
    }

    buffer_set_cursor(buffer, 0);
defer:
    if(sb.data) free(sb.data);
    if(fin) fclose(fin);
    return ret;
}

int ute_write(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    String_View sv = {0};

    if(buffer->file_name == NULL) {
        sv = read_command_line(ute, "Save file: ");
        if(sv.count > 0) buffer->file_name = sv_to_cstr(sv);
    }

    FILE *fout = fopen(buffer->file_name, "w");
    if(fout == NULL) return 1;

    int saved_cursor = buffer->cursor;
    int buf_size = buffer_size(buffer);

    buffer_set_cursor(buffer, buf_size);

    while(buf_size > 0) {
        int n = fwrite(buffer->data, sizeof(*buffer->data), buf_size, fout);
        UTE_ASSERT(n != 0, "ERROR: did not write anything to the file");
        buf_size -= n/sizeof(*buffer->data);
    }

    buffer_set_cursor(buffer, saved_cursor);
    buffer->dirty = 0;

    fclose(fout);
    return 0;
}

int open_file(Editor *ute, char *file_name) {
    int ret = 1;
    Buffer buffer = {0};

    UTE_ASSERT(file_name != NULL, "ERROR: cannot pass an empty file_name");
    buffer.file_name = file_name;

    String_View sv = { .data = ute->cwd, .count = strlen(ute->cwd) };

    ret = read_file(&buffer, sv);
    if (ret) {
        buffer_free(&ute->buffer);
        ute->buffer = buffer;
        ute->display.up_to_date = false;
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

Buffer *current_buffer(Editor *ute) {
//    UTE_ASSERT(0 , "ERROR: current_buffer not implemented");
    return &ute->buffer;
    /* if (ute->buffers.count == 0) { */
    /*     return NULL; */
    /* } */
    /* return &ute->buffers.data[ute->curr_buffer]; */
}

void buffers_next(Editor *ute) {
    (void) ute;
    UTE_ASSERT(0, "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer + 1) % ute->buffers.count;
}

void buffers_prev(Editor *ute) {
    (void) ute;
    UTE_ASSERT(0, "ERROR: buffers_next not implemented");
    //ute->curr_buffer = (ute->curr_buffer - 1 + ute->buffers.count) % ute->buffers.count;
}

char *sv_to_cstr(String_View sv) {
    String_Builder sb = {0};
    char *data = sv.data;
    size_t count = sv.count;
    ute_da_append_many(&sb, data, count);
    ute_da_append(&sb, 0);
    return sb.data;
}

void ute_search_word(Editor *ute) {
    Buffer *gb = current_buffer(ute);
    int saved_cursor = gb->cursor;
    String_View query = {0};
    int start = ute->command.cursor;

    int quit = 0;
    int forward = 0;
    int last_match = -1;
    size_t gb_size = buffer_size(gb);

    char *search_message = "Search: ";
    print_command_line(ute, search_message);
    while(1) {
        // NOTE: Start the search from the current position
        size_t cur_char = gb->cursor;
        buffer_set_cursor(gb, gb_size);

        int ch = getch();
        switch(ch) {
            case '\n':
                quit = 1;
                break;
            case KEY_CTRL('c'):
                // Reset the buffer when encounter C-c
                buffer_set_cursor(gb, saved_cursor);
                ute->command.cursor = start;
                return;
            case KEY_CTRL('f'):
                forward = 1;
                break;
            case 127:
            case KEY_BACKSPACE:
                buffer_remove(&ute->command);
                break;
            default:
                buffer_insert(&ute->command, ch);
                query.data = &ute->command.data[start];
                query.count = ute->command.cursor - start;
        }
        print_command_line(ute, search_message);
        if(quit) break;
        // Search in the buffer the word
        while(cur_char < gb_size) {
            if(forward && last_match == (int) cur_char) {
                cur_char++;
                continue;
            }
            if(cur_char + query.count < gb_size
                    && strncmp(&gb->data[cur_char], query.data, query.count) == 0) {
                ute->display.highlight_search = cur_char;
                ute->display.highlight_count = query.count;
                break;
            }
            cur_char++;
        }
        if(cur_char < gb_size) {
            last_match = cur_char;
            buffer_set_cursor(gb, cur_char);
            ute->display.up_to_date = false;
            update_display(ute);
            print_command_line(ute, search_message);
        }
    }
    buffer_set_cursor(gb, last_match);
    ute->command.cursor = start;
}

int ute_open(Editor *ute) {
    String_View sv;

    sv = read_command_line(ute, "Open file: ");
    if(sv.count == 0) return 0;
    char *file_name = sv_to_cstr(sv);
    return open_file(ute, file_name);
}


// TODO: Use a Buffer structure for the command line, to have automatic history
// TODO: Improve keybinding management
// TODO: C syntax highlighting
