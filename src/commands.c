#include "commands.h"

int ute_search_word(Editor *ute) {
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

int ute_open(Editor *ute) {
    String_View sv;

    sv = read_command_line(ute, "Open file: ");
    if(sv.count == 0) return 0;
    char *file_name = sv_to_cstr(sv);
    return open_file(ute, file_name);
}

int ute_command(Editor *ute) {
    String_View sv;

    sv = read_command_line(ute, ": ");
    if(sv.count == 0) return 0;
    UTE_ASSERT(0, "ute_command: not implemented");
    command_search_name(sv);
    return 1;
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
