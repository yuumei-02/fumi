#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "flags.h"
#include "lexer.h"
#include "parser.h"

i32 token_dump(const cstr file_path) {
   Lexer lexer = Lexer_new(file_path);

   Token token;
   do {
      token = Lexer_next(&lexer);
      
      Token_print(lexer.path, token);
      Token_free(token);
   } while (token.type != TT_Eof);

   Lexer_free(&lexer);
   return 0;
}

i32 compile(const cstr file_path, CompileFlags flags) {
   if (flags.token_dump)
      return token_dump(file_path);

   bool failure;
   Ast ast = Ast_parse_from_file_path(file_path, &failure);
   if (failure) return 1;

   if (flags.ast_dump) {
      Ast_print(ast);
   }
   
   Ast_free(&ast);
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("Usage: %s file.fum", argv[0]);
      eprintln("[!] Missing arguments");
      return 1;
   }

   CompileFlags flags = CompileFlags_default();
   Vector path_indexes = Vector_new(sizeof(i32));

   for (i32 i = 1; i < argc; ++i) {
      cstr_match(argv[i]) {
         ncstreq("--token-dump") flags.token_dump = true;
         cstreq("--ast-dump")    flags.ast_dump   = true;
         
         else {
            Vector_push(&path_indexes, &i);
         }
      }
   }

   foreach (path_indexes, i) {
      i32 argi = *(i32*) Vector_get(&path_indexes, i);
      if (compile(argv[argi], flags))
         return 1;
   }

   return 0;
}

