#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "buffer.h"
#include "commands.h"
#include "editor.h"
#include "utils.h"
#include "lexer.h"

#define is_printable(x) ((0x20 <= x && x <= 0xFF) || x == '\n' || x == '\t')

int manage_key(Editor *ute);

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
    init_pair(LITERAL_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(HIGHLIGHT_COLOR, COLOR_BLACK, COLOR_YELLOW);

    init_pair(MARK_SELECTION_COLOR, COLOR_BLACK, COLOR_GREEN);
    init_pair(STATUS_LINE_COLOR, COLOR_WHITE, COLOR_BLACK);

    getmaxyx(stdscr, ute.screen_height, ute.screen_width);
#if 0
    int ch, stop = 0;
    ch = getch();
    endwin();

    printf("%d\n", ch);
    return 0;
#else

    if(argc > 0) {
        const char *file_name = strdup(shift_args(&argc, &argv));
        String_Builder sb = {0};
        if(read_file(&sb, file_name)) {
            Buffer buffer = {0};
            buffer_insert_str(&buffer, sb.data, sb.count);
            buffer.file_name = (char *)file_name;

            buffer_set_cursor(&buffer, 0);
            ute_da_append(&ute.buffers, buffer);
            ute.curr_buffer = ute.buffers.count - 1;
        } else {
            // TODO: error reporting
        }
        if(sb.max_size > 0) free(sb.data);
    }

    if(ute.buffers.count == 0) {
        Buffer buffer = {0};

        buffer_set_cursor(&buffer, 0);
        ute_da_append(&ute.buffers, buffer);
        ute.curr_buffer = ute.buffers.count - 1;
    }

    int stop = 0;

    update_display(&ute);

    while (!stop) {
        stop = manage_key(&ute);
        update_display(&ute);
    }
    endwin();

    for(size_t i = 0; i < ute.buffers.count; i++)
        buffer_free(&ute.buffers.data[i]);
    free(ute.buffers.data);
    buffer_free(&ute.command);
    free(ute.display.data);
    free(ute.display.attr.data);
    return 0;
#endif
}

int manage_key(Editor *ute) {
    int ch = getch();
	
    Buffer *buffer = current_buffer(ute);
    if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, ute->screen_height, ute->screen_width);
        return 0;
    }

    switch(ute->mode) {
        case NORMAL_MODE:
            switch (ch) {
                case 'i':
                    ute->mode = INSERT_MODE;
                    break;
                case 'j':
                case KEY_DOWN:
                    buffer_next_line(buffer);
                    break;
                case 'k':
                case KEY_UP:
                    buffer_prev_line(buffer);
                    break;
                case 'l':
                case KEY_RIGHT:
                    buffer_right(buffer);
                    break;
                case 'h':
                case KEY_LEFT:
                    buffer_left(buffer);
                    break;
                case 'w':
                    buffer_forward_word(buffer);
                    break;
                case 'b':
                    buffer_backward_word(buffer);
                    break;
                case 'v':
                    // TODO: make a function to wrap this operation, so it can
                    // be called as a command
                    buffer->mark_position = buffer->cursor;
                    ute->display.up_to_date = false;
                    break;
                case ':':
                    editor_command(ute);
                    break;
                case 'g':
                {
                    if(buffer->lines.count > 0) {
                        int start = buffer->lines.data[0].start;
                        buffer_set_cursor(buffer, start);
                    }
                } break;
                case 'G':
                {
                    if(buffer->lines.count > 0) {
                        int last_line = buffer->lines.count-1;
                        int start = buffer->lines.data[last_line].start;
                        buffer_set_cursor(buffer, start);
                    }
                } break;
                case 'D':
                {
                    if(buffer->lines.count > 0) {
                        History_Item hi_item = {0};
                        int remove_start = buffer->mark_position;

                        int save_start, save_end;
                        int rx, ry, cx, cy;

                        // NOTE: use the remove side of the diff
                        buffer_posyx(buffer, remove_start, &ry, &rx);
                        buffer_posyx(buffer, buffer->cursor, &cy, &cx);
                        save_start = buffer->lines.data[ry].start;
                        save_end = buffer->lines.data[cy].end;
                        ute_da_append_many(&hi_item.remove, &buffer->sb.data[save_start], save_end - save_start);
                        hi_item.remove_start = save_start;

                        buffer_remove_selection(buffer);
                        buffer_parse_line(buffer);

                        // NOTE: use the insert side of the diff
                        buffer_posyx(buffer, buffer->cursor, &cy, &cx);

                        save_start = buffer->lines.data[cy].start;
                        save_end = buffer->lines.data[cy].end;
                        ute_da_append_many(&hi_item.insert, &buffer->sb.data[save_start], save_end - save_start);
                        hi_item.insert_start = save_start;

                        ute_da_append(&ute->history.undo_list, hi_item);
                        buffer->dirty = 1;
                        ute->display.up_to_date = false;
                    }
                } break;
                case 'u':
                {
                    if(ute->history.undo_list.count > 0) {
                        History_Item restore = ute->history.undo_list.data[ute->history.undo_list.count - 1];
                        if(restore.insert.count > 0) {
                            int mark_position = buffer->mark_position;

                            int restore_start = restore.insert_start;
                            buffer->mark_position = restore_start;
                            buffer_set_cursor(buffer, restore_start + restore.insert.count);
                            buffer_remove_selection(buffer);

                            buffer->mark_position = mark_position;
                        }
                        if(restore.remove.count > 0) {
                            int restore_start = restore.remove_start;
                            buffer_set_cursor(buffer, restore_start);
                            buffer_insert_str(buffer, restore.remove.data, restore.remove.count);
                        }
                        ute->history.undo_list.count--;
                        ute_da_append(&ute->history.redo_list, restore);
                        ute->display.up_to_date = false;
                    }

                } break;
                case KEY_CTRL('c'):
                    return 1;
                case KEY_CTRL('s'):
                    editor_write(ute);
                    break;
                case KEY_CTRL('o'):
                    if(!editor_open(ute)) {
                        // TODO: print error when is not possible to open the file
                    }
                    break;
                case KEY_CTRL('f'):
                    editor_search_word(ute);
                    break;
            }
            break;
        case INSERT_MODE:
        {
            switch (ch) {
                case KEY_ESCAPE:
                case KEY_CTRL('c'):
                    ute->mode = NORMAL_MODE;
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
                default:
                {
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
        } break;
    }
    return 0;
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    UTE_ASSERT(*argc > 0 && arg != NULL, "ERROR: no arguments provided");
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

// TODO: Use a Buffer structure for the command line, to have automatic history
// TODO: Improve keybinding management
// TODO: Implement simple modal editing
// TODO: Implement Emacs mode mechanism or another way to have commands specific for certain buffers
