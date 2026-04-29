#include <mcu/core.h>
#include <mcu/io.h>

#include "reporter.h"
#include "lexer.h"

void report_unexpected_token(const cstr path, Token token) {
   mcu_assert(path != nullptr, "path can't be null");

   println("%s:%zu:%zu:%zu: unexpected token \"%s\"",
      path, token.y, token.x, token.length, TokenType_to_cstr(token.type));
}

void report_unexpected_token_expected(const cstr path, Token token, TokenType expected) {
   mcu_assert(path != nullptr, "path can't be null");

   println("%s:%zu:%zu:%zu: unexpected token \"%s\", expected token: \"%s\"",
      path, token.y, token.x, token.length,
      TokenType_to_cstr(token.type),
      TokenType_to_cstr(expected));
}

