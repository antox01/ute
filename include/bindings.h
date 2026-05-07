#ifndef BINDINGS_H
#define BINDINGS_H

#include "utils.h"

// #define COMMAND_FUNC(name) Range name(Editor *)
// typedef COMMAND_FUNC(Motion_Func);

typedef struct editor Editor;

typedef Range Motion_Func(Editor *);
typedef void Operator_Func(Editor *, Range);

Motion_Func *bindings_get_motion(char ch);
Operator_Func *bindings_get_operator(char ch);

#endif // BINDINGS_H
