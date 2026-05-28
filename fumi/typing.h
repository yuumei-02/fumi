#pragma once

#include "ast.h"

void Ast_create_symbol_tables(Ast* self);
bool Ast_analyize_semantics(Ast* self);

