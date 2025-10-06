#include <stdio.h>
#include "utils.h"

char *sv_to_cstr(String_View sv) {
    String_Builder sb = {0};
    char *data = sv.data;
    size_t count = sv.count;
    ute_da_append_many(&sb, data, count);
    ute_da_append(&sb, 0);
    return sb.data;
}

int read_file(String_Builder *sb, const char *file_name) {
    size_t file_size;
    int ret = 1;

    FILE *fin = fopen(file_name, "r");
    char str_buf[MAX_STR_SIZE];

    if(fin == NULL) ret_defer(0);

    if(!get_file_size(fin, &file_size)) ret_defer(0);

    while(file_size > 0) {
        int n = fread(str_buf, sizeof(*str_buf), MAX_STR_SIZE, fin);
        file_size -= n;
        ute_da_append_many(sb, str_buf, n);
    }

defer:
    if(fin) fclose(fin);
    return ret;
}

int get_file_size(FILE *fin, size_t *size) {
    size_t start, end;
    start = ftell(fin);
    if(fseek(fin, 0, SEEK_END) < 0) return 0;
    end = ftell(fin);
    if(fseek(fin, start, SEEK_SET) < 0) return 0;
    *size = end;

    return 1;
}

