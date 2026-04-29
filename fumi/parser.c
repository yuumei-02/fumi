#include <mcu/core.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "parser.h"

typedef struct {
   bool panic_mode;
   bool failure;
   bool hard_failure;
} ParseState;

#define enter_panic() \
   state->panic_mode = true; \
   state->failure = true;

#define exit_panic() \
   state->panic_mode = false;

void parse_module(const cstr path, Ast* ast, ParseState* state) {
   Lexer lexer = Lexer_new(path, &state->hard_failure);
   if (state->hard_failure) return;

   Lexer_free(&lexer);
}

Ast Ast_parse_from_file_path(const cstr path, bool* failure) {
   mcu_assert(path != nullptr, "path can't be null");

   Ast self = {
      .AstNode_modules = Vector_new(sizeof(AstNode)),
      .AstNodes = Vector_new(sizeof(AstNode))
   };

   ParseState state = {0};
   parse_module(path, &self, &state);
   if (state.hard_failure || state.failure)
      goto failure;

   if (failure != nullptr) *failure = false;
   return self;

failure:
   if (failure != nullptr) *failure = true;
   return self;
}

