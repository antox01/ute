#ifndef COMMON_H
#define COMMON_H

#include <assert.h>

#define ret_defer(x) do { ret = x; goto defer; } while(0)


/* ute_da_append:
 * Macro to append an element to a dynamic array.
 */
#define ute_da(type, name) \
    typedef struct {\
        type *data;\
        size_t count;\
        size_t max_size;\
    } name;

#define ute_da_append(da,item) \
    do {\
        if((da)->count + 1 >= (da)->max_size) { \
            size_t max_size = (da)->count + 1; \
            (da)->data = realloc((da)->data, max_size * sizeof(*(da)->data)); \
            \
            assert((da)->data != NULL); \
            \
            (da)->max_size = max_size; \
        } \
        (da)->data[(da)->count] = (item); \
        (da)->count += 1; \
    } while(0)

/* ute_da_append_many:
 * Macro to append many elements to a dynamic array.
 */
#define ute_da_append_many(da,items,count) \
    do {\
        if((da)->count + (count) >= (da)->max_size) { \
            size_t max_size = (da)->count + (count); \
            (da)->data = realloc((da)->data, max_size * sizeof(*(da)->data)); \
            \
            if((da)->data == NULL) { \
                return; \
            } \
            (da)->max_size = max_size; \
        } \
        memcpy(&(da)->data[(da)->count], (items), (count)*sizeof(*(da)->data);\
        (da)->count += (count); \
    } while(0)

#endif // COMMON_H
