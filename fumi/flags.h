#pragma once

#include <mcu/types.h>

typedef struct {
   bool token_dump;
   bool ast_dump;
   bool doc_gen;
} CompileFlags;

CompileFlags CompileFlags_default();

