#include <mcu/core.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>

#include "lexer.h"

Lexer Lexer_new(const cstr path, bool* failure) {
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
      goto failure;
   }

   self.peek = fgetc(self.handle);
   if (self.peek == EOF && ferror(self.handle)) {
      eprintln("[!] Failed to read from file \"%s\", reason: \"%s\"", path, strerror(errno));
      goto failure;
   }

   self.accumulated = String_new();

   if (failure != nullptr) *failure = false;
   return self;

failure:
   if (failure != nullptr) *failure = true;
   return self;
}

void Lexer_free(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   String_free(&self->accumulated);
   if (fclose(self->handle)) {
      eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", self->path, strerror(errno));
   }

   *self = (Lexer) {0};
}

