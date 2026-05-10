#pragma once

#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"
#include "ast.h"

/// [failure] is allowed to be null.
Ast Ast_parse_from_file_path(const cstr path, bool* failure);
void Ast_free(Ast* self);

void Ast_print(Ast self);

