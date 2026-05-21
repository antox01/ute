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

char *shift_args(int *argc, char ***argv);

void render_display(Editor *ute) {
    int cy, cx;
    Buffer *buffer = current_buffer(ute);
    Display *display = &ute->display;

    int saved_cursor = buffer->cursor;
    buffer_posyx(buffer, saved_cursor, &cy, &cx);

    UTE_ASSERT(cx >= 0 && cy >= 0, "ERROR: got cx or cy negative");

    int width = ute->screen_width;
    int height = ute->screen_height - STATUS_LINE_SPACE;

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
            NCURSES_COLOR_T new_attribute = COLOR_PAIR(display->attr.data[curr_char]);
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

    update_display(&ute);
    render_display(&ute);

    while (!ute.quit) {
        int ch = getch();
        manage_key(&ute, ch);
        if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, ute.screen_height, ute.screen_width);
        }

        update_display(&ute);
        render_display(&ute);
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
