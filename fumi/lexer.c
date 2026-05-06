#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "lexer.h"

HashMap_hdr(TokenType)
HashMap_impl(TokenType)

bool Lexer_keyword_hashmap_defined = false;
// No need to free this hashmap as its supposed to live for the entire duration
// of the program anyway
HashMap(TokenType) keywords;

void Lexer_define_keyword_hashmap() {
   if (Lexer_keyword_hashmap_defined)
      return;

   keywords = HashMap_new(TokenType)();

   #define def_keyword(keyword, type) \
      HashMap_put(TokenType)(&keywords, keyword, type);

   def_keyword("procedure", TT_Procedure);
   def_keyword("return",    TT_Return);
   def_keyword("with",      TT_With);
   def_keyword("returns",   TT_Returns);
   def_keyword("begin",     TT_Begin);
   def_keyword("end",       TT_End);
   def_keyword("if",        TT_If);
   def_keyword("then",      TT_Then);
   def_keyword("else",      TT_Else);
   def_keyword("while",     TT_While);
   def_keyword("do",        TT_Do);
   def_keyword("break",     TT_Break);
   def_keyword("continue",  TT_Continue);

   #undef def_keyword
}

const cstr TokenType_to_cstr(TokenType self) {
   switch (self) {
      // Miscellaneous
      case TT_Eof: return "Eof";

      // Single char
      case TT_Colon:   return "Colon";
      case TT_Semicol: return "Semicol";
      case TT_Comma:   return "Comma";
      case TT_Equals:  return "Equals";
      case TT_Plus:    return "Plus";
      case TT_Min:     return "Min";
      case TT_Mul:     return "Mul";
      case TT_Div:     return "Div";
      case TT_NewLine: return "NewLine";
      case TT_Less:    return "Less";
      case TT_Great:   return "Great";
      case TT_LParen:  return "LParen";
      case TT_RParen:  return "RParen";

      // Double char
      case TT_DoubleEquals: return "DoubleEquals";
      case TT_LessEquals:   return "LessEquals";
      case TT_GreatEquals:  return "GreatEquals";
      case TT_NotEquals:    return "NotEquals";
      case TT_DoubleAnd:    return "DoubleAnd";
      case TT_DoublePipe:   return "DoublePipe";
      case TT_PlusEquals:   return "PlusEquals";
      case TT_MinEquals:    return "MinEquals";
      case TT_MulEquals:    return "MulEquals";
      case TT_DivEquals:    return "DivEquals";

      // Literals
      case TT_Identifier:    return "Identifier";
      case TT_IntLiteral:    return "IntLiteral";
      case TT_StringLiteral: return "StringLiteral";

      // Keywords
      case TT_Procedure: return "Procedure";
      case TT_Return:    return "Return";
      case TT_With:      return "With";
      case TT_Returns:   return "Returns";
      case TT_Begin:     return "Begin";
      case TT_End:       return "End";
      case TT_If:        return "If";
      case TT_Then:      return "Then";
      case TT_Else:      return "Else";
      case TT_While:     return "While";
      case TT_Do:        return "Do";
      case TT_Break:     return "Break";
      case TT_Continue:  return "Continue";
   }

   return "Unknown";
}

void Token_free(Token self) {
   switch (self.type) {
      case TT_StringLiteral: [[fallthrough]];
      case TT_Identifier: {
         String_free(&self.str_literal);
      } break;

      default: break;
   }
}

void Token_print(const cstr path, Token self) {
   printf("%s:%zu:%zu:%zu: %s", path, self.y, self.x, self.length, TokenType_to_cstr(self.type));

   switch (self.type) {
      case TT_StringLiteral: [[fallthrough]];
      case TT_Identifier:    println(" (%s)", self.str_literal.chars); break;
      case TT_IntLiteral:    println(" (%ld)", self.int_literal);      break;

      default: {
         printf("\n");
      }
   }
}

Lexer Lexer_new(const cstr path) {
   mcu_assert(path != nullptr, "path can't be null");

   Lexer self = {
      .x = 0,
      .y = 1,
      .mode = LM_Trim,
      .path = path
   };

   self.handle = fopen(path, "r");
   if (self.handle == nullptr) {
      eprintln("[!] Failed to open file \"%s\", reason: \"%s\"", path, strerror(errno));
      exit(errno);
   }

   self.peek = fgetc(self.handle);
   if (self.peek == EOF && ferror(self.handle)) {
      eprintln("[!] Failed to read from file \"%s\", reason: \"%s\"", path, strerror(errno));
      exit(errno);
   }

   self.accumulated = String_new();
   self.Token_undo_stack = Vector_new(sizeof(Token));
   Lexer_define_keyword_hashmap();

   return self;
}

void Lexer_free(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   String_free(&self->accumulated);
   Vector_free(&self->Token_undo_stack);
   if (fclose(self->handle)) {
      eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", self->path, strerror(errno));
   }

   *self = (Lexer) {0};
}

void Lexer_advance(Lexer* self) {
   if (self->current == '\n') {
      self->y += 1;
      self->x = 1;
   } else {
      self->x += 1;
   }

   self->current = self->peek;
   self->peek = fgetc(self->handle);

   if (self->peek == EOF && ferror(self->handle)) {
      eprintln("[!] Failed to read from file \"%s\", reason: \"%s\"", self->path, strerror(errno));
      exit(1);
   }
}

bool char_is_identifier_allowed(char self) {
   if (self >= 'a' && self <= 'z') return true;
   if (self >= 'A' && self <= 'Z') return true;
   if (self >= '0' && self <= '9') return true;
   if (self == '-' || self == '_') return true;
   return false;
}

Token Lexer_next(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   if (self->Token_undo_stack.length > 0) {
      return *(Token*) Vector_pop(&self->Token_undo_stack);
   }

   self->mode = LM_Trim;
   String_clear(&self->accumulated);

   Token token = {
      .x = self->x,
      .y = self->y,
      .length = 1,
      .type = TT_Eof
   };

   bool int_is_negative = false;

   #define return_single(token_type) \
      token.type = token_type; \
      return token

   #define return_double(token_type) \
      Lexer_advance(self); \
      token.type = token_type; \
      token.length = 2; \
      return token

   loop {
      Lexer_advance(self);
      if (self->current == EOF) break;

   reparse_char:
      switch (self->mode) {
         case LM_Trim: {
            token.x = self->x;
            token.y = self->y;

            switch (self->current) {
               case ':':  return_single(TT_Colon);
               case ';':  return_single(TT_Semicol);
               case ',':  return_single(TT_Comma);
               case '\n': return_single(TT_NewLine);
               case '(':  return_single(TT_LParen);
               case ')':  return_single(TT_RParen);

               case '+': {
                  switch (self->peek) {
                     case '=': return_double(TT_PlusEquals);
                     default:  return_single(TT_Plus);
                  }
               } break;

               case '-': {
                  if (self->peek >= '0' && self->peek <= '9') {
                     token.length += 1;
                     int_is_negative = true;
                     self->mode = LM_Integer;
                     break;
                  }

                  switch (self->peek) {
                     case '=': return_double(TT_MinEquals);
                     default:  return_single(TT_Min);
                  }
               } break;

               case '*': {
                  switch (self->peek) {
                     case '=': return_double(TT_MulEquals);
                     default:  return_single(TT_Mul);
                  }
               } break;

               case '/': {
                  switch (self->peek) {
                     case '/': self->mode = LM_Comment; break;
                     case '=': return_double(TT_DivEquals);
                     default:  return_single(TT_Div);
                  }
               } break;

               case '=': {
                  switch (self->peek) {
                     case '=': return_double(TT_DoubleEquals);
                     default:  return_single(TT_Equals);
                  }
               } break;

               case '<': {
                  switch (self->peek) {
                     case '=': return_double(TT_LessEquals);
                     default:  return_single(TT_Less);
                  }
               } break;

               case '>': {
                  switch (self->peek) {
                     case '=': return_double(TT_GreatEquals);
                     default:  return_single(TT_Great);
                  }
               } break;

               case '!': {
                  if (self->peek == '=') {
                     return_double(TT_NotEquals);
                  }
               } break;

               case '&': {
                  if (self->peek == '&') {
                     return_double(TT_DoubleAnd);
                  }
               } break;

               case '|': {
                  if (self->peek == '|') {
                     return_double(TT_DoublePipe);
                  }
               } break;

               case '"': {
                  self->mode = LM_String;
               } break;

               case ' ': break;

               default: {
                  if (self->current >= '0' && self->current <= '9') {
                     int_is_negative = false;
                     self->mode = LM_Integer;
                     goto reparse_char;
                  }

                  if (char_is_identifier_allowed(self->current)) {
                     self->mode = LM_Normal;
                     goto reparse_char;
                  }
               }
            }
         } break;

         case LM_Comment: {
            if (self->peek == EOF || self->peek == '\n') {
               self->mode = LM_Trim;
            }
         } break;

         case LM_Normal: {
            String_append(&self->accumulated, self->current);

            if (!char_is_identifier_allowed(self->peek)) {
               token.length = self->accumulated.length;

               TokenType* keyword = HashMap_get(TokenType)(&keywords, self->accumulated.chars);
               if (keyword == nullptr) {
                  token.type = TT_Identifier;
                  token.str_literal = String_clone(self->accumulated);
                  return token;
               }

               token.type = *keyword;
               return token;
            }
         } break;

         case LM_Integer: {
            token.int_literal *= 10;
            token.int_literal += self->current - '0';

            if (!(self->peek >= '0' && self->peek <= '9')) {
               if (int_is_negative) token.int_literal = -token.int_literal;
               token.type = TT_IntLiteral;
               return token;
            }

            token.length += 1;
         } break;

         case LM_String: {
            switch (self->current) {
               case '"': {
               return_string_literal:
                  token.type = TT_StringLiteral;
                  token.length = self->accumulated.length;
                  token.str_literal = String_clone(self->accumulated);
                  return token;
               }
               
               default: {
                  String_append(&self->accumulated, self->current);
               }
            }

            if (self->peek == EOF) {
               eprintln("%s:%zu:%zu: Unterminated string literal", self->path, token.y, token.x);
               goto return_string_literal;
            }
         } break;

         default: {
            panic("unreachable");
         }
      }
   }

   return token;
}

Token Lexer_peek(Lexer* self) {
   Token peek = Lexer_next(self);
   Lexer_undo(self, peek);

   return peek;
}

void Lexer_undo(Lexer* self, Token token) {
   mcu_assert(self != nullptr, "self can't be null");
   Vector_push(&self->Token_undo_stack, &token);
}

