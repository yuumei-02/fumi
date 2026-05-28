#pragma once

#include "ast.h"

bool Ast_create_symbol_tables(Ast* self);
bool Ast_analyize_semantics(Ast* self);

