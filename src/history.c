// Possible implementation
// - Using a diff structure
//   - Add a way to denote the start of a diff
//   - Should we use a backbuffer? Probably not needed if we just use the snapshot buffer as a backbuffer
//   - Insert/Delete type for a diff
//   - Store the line start and end
//   - Store the previous value of the string and the current one
//   - In case of the redo, change the types of the diff
//   - Should we implement an undotree or just a simple list? Let's do a simple list
//   - Think of adding an Arena to possibly optimize the allocations
//   - How do we tell when to start a diff in entering insert mode?
//      - When entering insert mode, we start the new history command, and then we just add a new character both on the Gap_Buffer and then on the snapshot buffer

#include "history.h"
#include "buffer.h"
#include "utils.h"

void history_free_command_list(Commands *list) {
    for(size_t i = 0; i < list->count; i++) {
        free(list->data[i].sb.data);
    }
    list->count = 0;
}

void history_insert_char(History *h, int cursor, char c) {
    if(h->redo_list.count > 0) history_free_command_list(&h->redo_list);
    if(h->undo_list.count > 0) {
        Command *command = &ute_da_last(&h->undo_list);
        if(command->kind == CMD_INSERT && command->cursor_end == cursor) {
            ute_da_append(&command->sb, c);
            command->cursor_end++;
            return;
        }
    }
    Command new_command = {
        .kind = CMD_INSERT,
        .cursor_start = cursor,
        .cursor_end = cursor+1,
    };
    ute_da_append(&new_command.sb, c);
    ute_da_append(&h->undo_list, new_command);
}

void history_delete_char(History *h, int cursor, char c) {
    if(h->redo_list.count > 0) history_free_command_list(&h->redo_list);
    if(h->undo_list.count > 0) {
        Command *command = &ute_da_last(&h->undo_list);
        if(command->kind == CMD_DELETE && command->cursor_start == cursor) {
            ute_da_insert_first(&command->sb, c);
            command->cursor_start--;
            return;
        }
    }
    Command new_command = {
        .kind = CMD_DELETE,
        .cursor_start = cursor-1,
        .cursor_end = cursor,
    };
    ute_da_insert_first(&new_command.sb, c);
    ute_da_append(&h->undo_list, new_command);
}

bool history_undo(History *h, void *buffer) {
    Buffer *b = buffer;
    if(h->undo_list.count == 0) return false;
    Command last = ute_da_last(&h->undo_list);
    switch(last.kind) {
        case CMD_INSERT:
            {
                b->mark_position = last.cursor_start;
                buffer_set_cursor(b, last.cursor_end);
                buffer_remove_selection(b);
            } break;
        case CMD_DELETE:
            {
                int saved_cursor = b->cursor;
                buffer_set_cursor(b, last.cursor_start);
                buffer_insert_str(b, last.sb.data, last.sb.count);
                buffer_set_cursor(b, last.cursor_start < saved_cursor ? last.sb.count + saved_cursor : saved_cursor);
            } break;
        default:
            UTE_ASSERT(0, "ERROR: Command received command not handled");
    }
    ute_da_append(&h->redo_list, last);
    h->undo_list.count--;
    return true;
}

bool history_redo(History *h, void *buffer) {
    Buffer *b = buffer;
    if(h->redo_list.count == 0) return false;

    Command last = ute_da_last(&h->redo_list);
    switch(last.kind) {
        case CMD_INSERT:
            {
                buffer_set_cursor(b, last.cursor_start);
                buffer_insert_str(b, last.sb.data, last.sb.count);
            } break;
        case CMD_DELETE:
            {
                int saved_cursor = b->cursor;
                b->mark_position = last.cursor_start;
                buffer_set_cursor(b, last.cursor_end);
                buffer_remove_selection(b);
                buffer_set_cursor(b, last.cursor_start < saved_cursor ? saved_cursor - last.sb.count: saved_cursor);
            } break;
        default:
            UTE_ASSERT(0, "ERROR: Command received command not handled");
    }
    ute_da_append(&h->undo_list, last);
    h->redo_list.count--;
    return true;
}

void history_free(History *h) {
    history_free_command_list(&h->redo_list);
    history_free_command_list(&h->undo_list);
    free(h->redo_list.data);
    free(h->undo_list.data);
}
