#ifndef COMMAND_H
#define COMMAND_H

#include "editor.h"
#include "utils.h"

#define COMMAND_FUNC(name) int name(Editor *)
typedef COMMAND_FUNC(Command_Func);

Command_Func *command_search_name(String_View sv);

#endif//COMMAND_H
