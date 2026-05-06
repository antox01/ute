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

typedef struct {
    int pos;
    char *name;
} Command_Index;

typedef struct {
    Command_Index *data;
    size_t count;
    size_t max_size;
} Completion_Candidates;

bool starts_with(char *src, size_t src_len, char *match, size_t match_len) {
    if(match_len > src_len) return false;
    for(size_t i = 0; i < match_len; i++) {
        if(src[i] != match[i]) return false;
    }
    return true;
}

void commands_get_completion_candidate(Completion_Candidates *cc, String_View sv) {
    for(size_t i = 0; i < ARRAY_LEN(commands); i++) {
        char *cname = commands[i].name;
        if(starts_with(cname, strlen(cname), sv.data, sv.count)) {
            Command_Index index = (Command_Index){ .pos = i, .name = cname };
            ute_da_append(cc, index);
        }
    }
}

Command_Func *command_search_name(String_View sv) {
    Completion_Candidates cc = {0};
    Command_Func *res = NULL;
    commands_get_completion_candidate(&cc, sv);
    UTE_ASSERT(cc.count <= 1, "TODO: multiple completion candidates not supported");
    if(cc.count == 1) res = commands[cc.data[0].pos].func;
    
    free(cc.data);
    return res;
}
