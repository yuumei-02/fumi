#pragma once

#include <mcu/types.h>

typedef struct {
   bool token_dump;
   bool ast_dump;
} CompileFlags;

CompileFlags CompileFlags_default();

