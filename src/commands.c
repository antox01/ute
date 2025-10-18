#include "commands.h"
#include "editor.h"
#include "utils.h"

struct {
    char *short_name;
    char *name;
    Command_Func *func;
} commands[] = {
    { .short_name = "s", .name = "search", .func = editor_search_word},
    { .short_name = "o", .name = "open", .func = editor_open},
    { .short_name = "w", .name = "write", .func = editor_write},
    { .short_name = "q", .name = "quit", .func = editor_quit},
    { .short_name = "bn", .name = "bnext", .func = editor_buffers_next},
    { .short_name = "bp", .name = "bprev", .func = editor_buffers_prev},
    { .short_name = "rs", .name = "remove-selection", .func = editor_remove_selection},
};

Command_Func *command_search_name(String_View sv) {
    for(size_t i = 0; i < ARRAY_LEN(commands); i++) {
        char *cname = commands[i].name;
        char *cshort = commands[i].short_name;
        if(strlen(cshort) == sv.count && strncmp(cshort, sv.data, sv.count) == 0) {
            return commands[i].func;
        }
        if(strlen(cname) == sv.count && strncmp(cname, sv.data, sv.count) == 0) {
            return commands[i].func;
        }
    }
    return NULL;
}
