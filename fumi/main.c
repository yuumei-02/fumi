#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "lexer.h"

i32 compile(const cstr file_path) {
   bool failure;
   Lexer lexer = Lexer_new(file_path, &failure);
   if (failure) return 1;

   Lexer_free(&lexer);
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("Usage: %s file.fum", argv[0]);
      eprintln("[!] Missing arguments");
      return 1;
   }

   for (i32 i = 1; i < argc; ++i) {
      if (compile(argv[i])) return 1;
   }

   return 0;
}

