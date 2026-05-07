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

    while (!ute.quit) {
        manage_key(&ute);
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
