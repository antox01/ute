#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <assert.h>

#define DEFAULT_COLOR         1
#define KEYWORD_COLOR         2
#define TYPE_COLOR            3
#define COMMENT_COLOR         4
#define PREPROC_COLOR         5
#define LITERAL_COLOR         6
#define HIGHLIGHT_COLOR       9
#define MARK_SELECTION_COLOR 10
#define STATUS_LINE_COLOR    16

#define STATUS_LINE_SPACE 2
#define STATUS_LINE_RIGHT_CHAR 20
#define TAB_TO_SPACE 4
#define EXPAND_TAB false


#define KEY_CTRL(x) (x & 0x1F)
#define KEY_ESCAPE 0x1B

#define MAX_STR_SIZE 256

#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif


#define ret_defer(x) do { ret = x; goto defer; } while(0)

#define max(x,y) (x) > (y) ? (x) : (y)

#define ARRAY_LEN(arr) (sizeof((arr))/sizeof((arr)[0]))

#define UTE_ASSERT(cond, msg) \
    do {\
        if((cond)) break;\
        endwin();\
        fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, msg); \
        abort();\
    } while (0)

#define UTE_DA_INIT_CAP 64

#define ute_da_reserve(da, expected_size) \
    do { \
        if((expected_size) > (da)->max_size){ \
            if((da)->max_size == 0) { \
                (da)->max_size = UTE_DA_INIT_CAP; \
            } \
            while(expected_size > (da)->max_size) { \
                (da)->max_size *= 2; \
            } \
            (da)->data = realloc((da)->data, (da)->max_size * sizeof(*(da)->data)); \
            UTE_ASSERT((da)->data != NULL, "ERROR: not enough memory"); \
        } \
    } while(0)

/* ute_da_append:
 * Macro to append an element to a dynamic array.
 */
#define ute_da_append(da,item) \
    do {\
        ute_da_reserve((da), (da)->count + 1);\
        (da)->data[(da)->count] = (item); \
        (da)->count += 1; \
    } while(0)

/* ute_da_append_many:
 * Macro to append many elements to a dynamic array.
 */
#define ute_da_append_many(da, items, size) \
    do {\
        ute_da_reserve((da), (da)->count + (size));\
        memcpy(&(da)->data[(da)->count], (items), (size)*sizeof(*(da)->data));\
        (da)->count += (size); \
    } while(0)

typedef struct {
    char *data;
    size_t count;
    size_t max_size;
} String_Builder;

typedef struct {
    char *data;
    size_t count;
} String_View;

char *sv_to_cstr(String_View sv);

// File management functions
int read_file(String_Builder *sb, const char *file_name);
int get_file_size(FILE *fin, size_t *size);

#endif // COMMON_H
