#include "buffer.h"
#include "commands.h"
#include "editor.h"
#include "lexer.h"
#include "utils.h"

Buffer *current_buffer(Editor *ute) {
    if(ute->buffers.count > 0)
        return &ute->buffers.data[ute->curr_buffer];
    return NULL;
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


int editor_search_word(Editor *ute) {
    Buffer *gb = current_buffer(ute);
    int saved_cursor = gb->cursor;
    String_View query = {0};
    int start = ute->command.cursor;

    int quit = 0;
    int forward = 0;
    int last_match = -1;
    size_t gb_size = buffer_size(gb);

    // NOTE: Start the search from the current position
    size_t cur_char = gb->cursor;

    char *search_message = "Search: ";
    print_command_line(ute, search_message);
    while(1) {
        int ch = getch();
        switch(ch) {
            case '\n':
                quit = 1;
                break;
            case KEY_CTRL('c'):
                // Reset the buffer when encounter C-c
                buffer_set_cursor(gb, saved_cursor);
                ute->command.cursor = start;
                return 0;
            case KEY_CTRL('f'):
                forward = 1;
                break;
            case 127:
            case KEY_BACKSPACE:
                buffer_remove(&ute->command);
                break;
            default:
                buffer_insert(&ute->command, ch);
        }
        query.data = &ute->command.data[start];
        query.count = ute->command.cursor - start;
        print_command_line(ute, search_message);
        if(quit) break;

        // Search in the buffer the word
        while(cur_char < gb_size) {
            if(forward && last_match == (int) cur_char) {
                cur_char++;
                continue;
            }
            if(cur_char + query.count < gb_size
                    && strncmp(&gb->sb.data[cur_char], query.data, query.count) == 0) {
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
            forward = 0;
            update_display(ute);
            print_command_line(ute, search_message);
        } else if(forward){
            cur_char = saved_cursor;
            forward = 0;
        }
    }
    buffer_set_cursor(gb, last_match);
    ute->command.cursor = start;
    return 0;
}

int editor_open(Editor *ute) {
    int ret = 1;
    String_View sv;

    sv = read_command_line(ute, "Open file: ");
    if(sv.count == 0) return 0;
    char *file_name = sv_to_cstr(sv);
    String_Builder sb = {0};
    if(read_file(&sb, file_name)) {
        Buffer buffer = {0};
        buffer_insert_str(&buffer, sb.data, sb.count);
        buffer.file_name = file_name;
        buffer_set_cursor(&buffer, 0);
        ute_da_append(&ute->buffers, buffer);
        ute->curr_buffer = ute->buffers.count - 1;
        ute->display.up_to_date = false;
    } else {
        // TODO: add error reporting
        ret = 0;
    }

    if(sb.max_size > 0) free(sb.data);
    return ret;
}

int editor_command(Editor *ute) {
    String_View sv;

    sv = read_command_line(ute, ": ");
    if(sv.count == 0) return 0;
    Command_Func *func = command_search_name(sv);
    if(func != NULL) {
        func(ute);
    } else {
        // TODO: error reporting
    }
    return 1;
}

int editor_write(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    String_View sv = {0};

    if(buffer->file_name == NULL) {
        sv = read_command_line(ute, "Save file: ");
        if(sv.count > 0) buffer->file_name = sv_to_cstr(sv);
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
                        display->attr.data[j + l.token.start] = COLOR_PAIR(color);
                        j++;
                    }
                }
            }
        }

        // NOTE: displaying the start of the mark for the selection
        if(display->attr.count > 0) display->attr.data[buffer->mark_position] = COLOR_PAIR(MARK_SELECTION_COLOR);


        // Highlight the searched character
        size_t hl_it = 0;
        while(hl_it < ute->display.highlight_count) {
            display->attr.data[hl_it + ute->display.highlight_search] = COLOR_PAIR(HIGHLIGHT_COLOR);
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

    int cur_x = 0;
    int cur_y = 0;

    display->count = 0;
    NCURSES_COLOR_T active_attribute = COLOR_PAIR(DEFAULT_COLOR);
    attrset(active_attribute);
    size_t i = 0;
    while(i < (size_t) height && i + buffer->sy < buffer->lines.count) {
        Line line = buffer->lines.data[i+buffer->sy];
        move(i, 0);
        size_t curr_char = line.start + buffer->sx;
        int j = 0;
        while(j < width && curr_char < line.end) {
            NCURSES_COLOR_T new_attribute = display->attr.data[curr_char];
            if(new_attribute != active_attribute) {
                active_attribute = new_attribute;
                addnstr(display->data, display->count);
                display->count = 0;
                attrset(new_attribute);
            }
            if(buffer->sb.data[curr_char] == '\t') {
                for(int ntab = 0; ntab < TAB_TO_SPACE; ntab++)
                    ute_da_append(display, ' ');
            } else ute_da_append(display, buffer->sb.data[curr_char]);

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
        if(buffer->sb.data[curr_char] == '\t') cx += TAB_TO_SPACE - 1;
    }

    buffer_set_cursor(buffer, saved_cursor);
    attroff(COLOR_PAIR(active_attribute));
    attron(COLOR_PAIR(STATUS_LINE_COLOR));

    //TODO: update_display managing the reset of the cursor
    print_status_line(ute);
    print_command_line(ute, "");
    refresh();

    cur_y = cy - buffer->sy;
    cur_x = cx - buffer->sx;
    if(cur_y >= height) cur_y = height - 1;
    move(cur_y, cur_x);
    refresh();
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

void print_status_line(Editor *ute) {
    Buffer *buffer = current_buffer(ute);
    int sline_pos = ute->screen_height - STATUS_LINE_SPACE;
    int sline_right_start = ute->screen_width - STATUS_LINE_RIGHT_CHAR;
    char str[MAX_STR_SIZE] = {0};
    int left_len = 0;
    int cy, cx;

    buffer_posyx(buffer, buffer->cursor, &cy, &cx);
    if (buffer->file_name) {
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", buffer->file_name);
    } else {
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", "[New File]");
    }
    if(buffer->dirty && left_len < MAX_STR_SIZE - 1)
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", " [+]");

    if(ute->mode == INSERT_MODE && left_len < MAX_STR_SIZE - 1)
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", " (INSERT)");
    else if(ute->mode == NORMAL_MODE && left_len < MAX_STR_SIZE - 1)
        left_len += snprintf(&str[left_len], MAX_STR_SIZE, "%s", " (NORMAL)");


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
        Command command = {0};
        command.kind = CMD_DELETE;
        int mark_position = buffer->mark_position;
        command.del.cursor_start = mark_position;

        ute_da_append_many(&command.del.sb, &buffer->sb.data[mark_position], buffer->cursor - mark_position);
        command.del.cursor_end = buffer->cursor;

        buffer_remove_selection(buffer);

        ute_da_append(&ute->history.undo_list, command);
        buffer->dirty = 1;
        ute->display.up_to_date = false;
    }
    return 0;
}
