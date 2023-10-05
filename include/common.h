#ifndef COMMON_H
#define COMMON_H

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
            (da)->data = realloc((da)->data, max_size * sizeof(char)); \
            \
            if((da)->data == NULL) { \
                return; \
            } \
            (da)->max_size = max_size; \
        } \
        (da)->data[(da)->count] = (item); \
        (da)->count += 1; \
    } while(0)

#define ute_da_append_many(da,items,size) \
    do {\
        if((da)->count + (size) >= (da)->max_size) { \
            size_t max_size = (da)->count + (size); \
            (da)->data = realloc((da)->data, max_size * sizeof(char)); \
            \
            if((da)->data == NULL) { \
                return; \
            } \
            (da)->max_size = max_size; \
        } \
        memcpy(&(da)->data[(da)->count], (items), (size));\
        (da)->count += (size); \
    } while(0)

#endif // COMMON_H
