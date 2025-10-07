#include "commands.h"
#include "editor.h"
#include "utils.h"

struct {
    char *name;
    Command_Func *func;
} commands[] = {
    { .name = "search", .func = editor_search_word},
    { .name = "open", .func = editor_open},
    { .name = "write", .func = editor_write},
    { .name = "bnext", .func = editor_buffers_next},
    { .name = "bprev", .func = editor_buffers_prev},
};

Command_Func *command_search_name(String_View sv) {
    for(size_t i = 0; i < ARRAY_LEN(commands); i++) {
        char *cname = commands[i].name;
        if(strlen(cname) == sv.count && strncmp(cname, sv.data, sv.count) == 0) {
            return commands[i].func;
        }
    }
    return NULL;
}
