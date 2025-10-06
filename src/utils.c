#include "utils.h"

char *sv_to_cstr(String_View sv) {
    String_Builder sb = {0};
    char *data = sv.data;
    size_t count = sv.count;
    ute_da_append_many(&sb, data, count);
    ute_da_append(&sb, 0);
    return sb.data;
}

