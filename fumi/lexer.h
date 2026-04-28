#include <mcu/core.h>
#include <mcu/io.h>

typedef enum {
   LM_Trim,
   LM_Normal,
   LM_Comment,
   LM_IntLiteral,
} LexerMode;

typedef struct {
   usize x;
   usize y;

   i32 current;
   i32 peek;
   LexerMode mode;
   String accumulated;

   const cstr path;
   FILE* handle;
} Lexer;

/// [failure] is allowed to be null
Lexer Lexer_new(const cstr path, bool* failure);
void Lexer_free(Lexer* self);

