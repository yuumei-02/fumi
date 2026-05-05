#pragma once

#include <mcu/core.h>
#include <mcu/io.h>

// @note: don't forget to update Token_free when adding new token types
// @todo: bad token
typedef enum {
   // Miscellaneous
   TT_Eof,

   // Single char
   TT_Colon,  TT_Semicol,
   TT_Equals, TT_NewLine,
   TT_Plus,   TT_Min,
   TT_Mul,    TT_Div,
   TT_Less,   TT_Great,

   // Double char
   TT_DoubleEquals, TT_NotEquals,
   TT_LessEquals,   TT_GreatEquals,
   TT_DoubleAnd,    TT_DoublePipe,
   TT_PlusEquals,   TT_MinEquals,
   TT_MulEquals,    TT_DivEquals,

   // Literals
   TT_Identifier,
   TT_IntLiteral,

   // Keywords
   TT_Procedure, TT_Return,
   TT_Begin,     TT_End,
   TT_If,        TT_Then,
   TT_While,     TT_Do,
   TT_Break,     TT_Continue,
} TokenType;

typedef struct {
   usize x;
   usize y;
   usize length;
   
   TokenType type;
   union {
      String str_literal;
      i64 int_literal;
   };
} Token;

typedef enum {
   LM_Trim,
   LM_Normal,
   LM_Comment,
   LM_Integer,
} LexerMode;

typedef struct {
   usize x;
   usize y;

   i32 current;
   i32 peek;
   LexerMode mode;
   String accumulated;
   Vector Token_undo_stack;

   const cstr path;
   FILE* handle;
} Lexer;

const cstr TokenType_to_cstr(TokenType self);

void Token_free(Token self);
void Token_print(const cstr path, Token self);

Lexer Lexer_new(const cstr path);
void Lexer_free(Lexer* self);

Token Lexer_next(Lexer* self);
Token Lexer_peek(Lexer* self);
void Lexer_undo(Lexer* self, Token token);

