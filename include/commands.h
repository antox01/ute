#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"

#define COMMAND_FUNC(name) int name(Editor *)
typedef COMMAND_FUNC(Command_Func);

Command_Func *command_search_name(String_View sv);

// TODO: make this functions commands of the editor
// to be called when it is possible to input commands
// in the command buffer
COMMAND_FUNC(ute_search_word);
COMMAND_FUNC(ute_open);
COMMAND_FUNC(ute_write);
COMMAND_FUNC(ute_command);

#endif//COMMAND_H
