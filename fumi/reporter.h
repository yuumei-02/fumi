#pragma once

#include <mcu/types.h>

#include "lexer.h"

void report_unexpected_token(const cstr path, Token token);
void report_unexpected_token_expected(const cstr path, Token token, TokenType expected);

