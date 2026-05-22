#include "buffer.h"
#include "bindings.h"
#include "commands.h"
#include "editor.h"
#include "lexer.h"
#include "utils.h"

Buffer *current_buffer(Editor *ute) {
    if(ute->buffers.count > 0 && (size_t) ute->curr_buffer < ute->buffers.count)
        return &ute->buffers.data[ute->curr_buffer];
    return NULL;
}

int editor_search_word(Editor *ute) {
    (void) ute;
    UTE_ASSERT(false, "TODO: editor_search_word");
}

int editor_open(Editor *ute) {
    (void) ute;
    UTE_ASSERT(false, "TODO: editor_open");
}

int editor_command(Editor *ute) {
    (void) ute;
    UTE_ASSERT(false, "TODO: editor_command");
    return 1;
}

int editor_write(Editor *ute) {
    Buffer *buffer = current_buffer(ute);

    if(buffer->file_name == NULL) {
        UTE_ASSERT(false, "TODO: Prompt not implemented");
        return 1;
    }

    FILE *fout = fopen(buffer->file_name, "w");
    if(fout == NULL) return 1;

    int buf_size = buffer_size(buffer);
    if(buffer->sb.count != (size_t)buf_size) buffer_parse_line(buffer);

    while(buf_size > 0) {
        int n = fwrite(buffer->sb.data, sizeof(*buffer->sb.data), buf_size, fout);
        UTE_ASSERT(n != 0, "ERROR: did not write anything to the file");
        buf_size -= n/sizeof(*buffer->sb.data);
    }

    buffer->dirty = 0;

    fclose(fout);
    return 0;
}

int editor_quit(Editor *ute) {
    // TODO: manage all dirty buffers to avoid closing without saving them

    ute->quit = 1;
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
        for (size_t j = 0; j < (size_t) buffer_size(buffer); j++) ute_da_append(&display->attr, COLOR_PAIR(DEFAULT_COLOR));

        if(buffer->file_name) {
            size_t flen = strlen(buffer->file_name);

            // TODO: find a better way to store the file type of the buffer
            if(flen > 2 && strcmp(&buffer->file_name[flen - 2], ".c") == 0) {
                Lexer l = lexer_init(buffer->sb.data, buffer_size(buffer));
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
                            color = LITERAL_COLOR;
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
            }
        }

        // NOTE: displaying the start of the mark for the selection
        if(display->attr.count > 0) display->attr.data[buffer->mark_position] = MARK_SELECTION_COLOR;


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

    if(cy < buffer->sy) buffer->sy = cy;
    else if(height <= cy - buffer->sy) buffer->sy = cy - height + 1;

    if(cx < buffer->sx) buffer->sx = cx;
    if(width <= cx - buffer->sx) buffer->sx = cx - width + 1;
    buffer_set_cursor(buffer, saved_cursor);
}

int editor_buffers_next(Editor *ute) {
    int next_buffer = ute->curr_buffer + 1;
    int buffers_count = ute->buffers.count;
    ute->curr_buffer = ((next_buffer % buffers_count) + buffers_count) % buffers_count;
    ute->display.up_to_date = false;
    return 0;
}

int editor_buffers_prev(Editor *ute) {
    int next_buffer = ute->curr_buffer - 1;
    int buffers_count = ute->buffers.count;
    ute->curr_buffer = ((next_buffer % buffers_count) + buffers_count) % buffers_count;
    ute->display.up_to_date = false;
    return 0;
}

int editor_remove_selection(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    if(buffer->lines.count > 0) {
        int mark_position = buffer->mark_position;
        history_delete_string(&buffer->history, buffer->cursor, &buffer->sb.data[mark_position], buffer->cursor - mark_position);
        buffer_remove_selection(buffer);

        buffer->dirty = 1;
        ute->display.up_to_date = false;
    }
    return 0;
}

int editor_undo(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    if(!history_undo(&buffer->history, buffer)) return 0;
    ute->display.up_to_date = false;
    return 1;
}

int editor_redo(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    if(!history_redo(&buffer->history, buffer)) return 0;
    ute->display.up_to_date = false;
    return 1;
}

void handle_normal_mode(Editor *ute, int ch) {
    Buffer *buffer = current_buffer(ute);
    if(ute->normal_state == NORMAL_STATE_IDLE) {
        Operator_Func *operator = bindings_get_operator(ch);
        if(operator != NULL) {
            ute->operator = operator;
            ute->normal_state = NORMAL_STATE_OPERATION;
            return;
        }
        Motion_Func *motion = bindings_get_motion(ch);
        if(motion != NULL) {
            Range range = motion(ute);
            buffer_set_cursor(buffer, range.end);
            return;
        }
        switch (ch) {
            case 'i':
                ute->mode = INSERT_MODE;
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
            case 'v':
                // TODO: make a function to wrap this operation, so it can
                // be called as a command
                buffer->mark_position = buffer->cursor;
                ute->display.up_to_date = false;
                break;
            case ':':
                UTE_ASSERT(false, "TODO: Prompt not implemented");
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
                    editor_remove_selection(ute);
                } break;
            case 'u':
                {
                    editor_undo(ute);
                } break;
            case 'r':
                {
                    editor_redo(ute);
                } break;
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
    } else if(ute->normal_state == NORMAL_STATE_OPERATION) {
        UTE_ASSERT(ute->operator != NULL, "ute->operator cannot be null");
        Motion_Func *motion = bindings_get_motion(ch);
        if(motion != NULL) {
            Range range = motion(ute);
            ute->operator(ute, range);
        }
        ute->normal_state = NORMAL_STATE_IDLE;
    }
}

void handle_insert_mode(Editor *ute, int ch) {
    Buffer *buffer = current_buffer(ute);
    switch (ch) {
        case KEY_ESCAPE:
        case KEY_CTRL('c'):
            ute->mode = NORMAL_MODE;
            break;
            // case KEY_DOWN:
            //     buffer_next_line(buffer);
            //     break;
            // case KEY_UP:
            //     buffer_prev_line(buffer);
            //     break;
            // case KEY_RIGHT:
            //     buffer_right(buffer);
            //     break;
            // case KEY_LEFT:
            //     buffer_left(buffer);
            //     break;
        case KEY_DC:
            {
                // buffer_right(buffer);
                // Command *command = &buffer->history.current;
                // if(command->kind != CMD_DELETE) {
                //     if(command->kind != CMD_NONE) {
                //         ute_da_append(&buffer->history.undo_list, *command);
                //         *command = (Command){0};
                //     }
                //     command->kind = CMD_DELETE;
                //     command->cursor_start = buffer->cursor - 1;
                //     command->cursor_end = buffer->cursor - 1;
                // }
                // ute_da_append(&command->sb, buffer->data[buffer->cursor-1]);
                // command->cursor_end++;
                // buffer_remove(buffer);
                // buffer->dirty = 1;
                // ute->display.up_to_date = false;
                Operator_Func *operator = bindings_get_operator('d');
                Motion_Func *motion = bindings_get_motion('l');
                operator(ute, motion(ute));
            } break;
        case 127:
        case KEY_BACKSPACE:
            {
                history_delete_char(&buffer->history, buffer->cursor, buffer->data[buffer->cursor - 1]);
                buffer_remove(buffer);
                buffer->dirty = 1;
                ute->display.up_to_date = false;
            } break;
        default:
            {
                // TODO: Combine the insert actions to be only one and being limited
                // to a String Builder

                if(is_printable(ch)) {
                    // Convert tab key to multiple spaces
                    if(EXPAND_TAB && ch == '\t') {
                        for(int i = 0; i < TAB_TO_SPACE; i++) {
                            history_insert_char(&buffer->history, buffer->cursor, ' ');
                            buffer_insert(buffer, ' ');
                        }
                    } else {
                        history_insert_char(&buffer->history, buffer->cursor, ch);
                        buffer_insert(buffer, ch);
                    }
                    buffer->dirty = 1;
                    ute->display.up_to_date = false;
                }
            }
    }
}

void handle_command_mode(Editor *ute, int ch) {
    (void) ute;
    (void) ch;
    UTE_ASSERT(false, "TODO: handle_command_mode not implemented");
}

int manage_key(Editor *ute, int ch) {
    switch(ute->mode) {
        case NORMAL_MODE: handle_normal_mode(ute, ch); break;
        case INSERT_MODE: handle_insert_mode(ute, ch); break;
        case COMMAND_MODE: handle_command_mode(ute, ch); break;
    }
    return 0;
}

