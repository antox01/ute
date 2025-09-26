#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <ncurses.h>
#include <assert.h>

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



#endif // COMMON_H
