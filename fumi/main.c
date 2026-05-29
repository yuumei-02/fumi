#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include <time.h>

#include "flags.h"
#include "lexer.h"
#include "parser.h"
#include "typing.h"
#include "doc_gen.h"

const cstr version_str = "0.0.1";

void version() {
   println("The bootstrap compiler for the fumi programming language.\n"
           "version: %s", version_str);
}

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
   i32 result = 0;

   if (flags.token_dump)
      return token_dump(file_path);

   time_t start = clock();
   bool failure;
   Ast ast = Ast_parse_from_file_path(file_path, &failure);
   if (failure) return 1;
   time_t end = clock();
   double front_end_time = (double) (end - start) / CLOCKS_PER_SEC;

   start = clock();
   if (Ast_create_symbol_tables(&ast)) result = 1;
   if (Ast_analyize_semantics(&ast)) result = 1;
   end = clock();
   double middle_end_time = (double) (end - start) / CLOCKS_PER_SEC;

   if (flags.ast_dump) {
      Ast_print(ast);
   }

   if (flags.doc_gen) {
      result = Ast_doc_gen(&ast);
   }

   println("Compilation statistics");
   println("Target: x86-64 Linux");
   println("frontend   : %.6lfs", front_end_time);
   println("middle-end : %.6lfs", middle_end_time);
   if (result == 0) {
      println("[i] Compilation successfull");
   } else {
      println("[!] Compilation failed");
   }
   Ast_free(&ast);
   return result;
}

void help() {
   printf(
      "The bootstrap compiler for the fumi programming language.\n"
      "\n"
      "Usage:\n"
      "   fumi <input-files> <?flags>\n"
      "\n"
      "Flags:\n"
      "   --token-dump   Stop after the lexical analysis phase and output the tokens to stdout.\n"
      "   --ast-dump     Stop after the semantic analysis phase and output the ast and other data to stdout.\n"
      "   --doc-gen      Instead of compilation, generate documentation for the input files. @note: Not yet implemented\n"
      "   --version      Print the current compiler version."
      "   --help         This help message.\n"
      "\n"
      "See \"fumi help <?command>\" for more information on a specific command. @note: Not yet implemented.\n");
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      help();
      eprintln("[!] Missing arguments");
      return 1;
   }

   CompileFlags flags = CompileFlags_default();
   Vector path_indexes = Vector_new(sizeof(i32));

   for (i32 i = 1; i < argc; ++i) {
      cstr_match(argv[i]) {
         ncstreq("--token-dump") flags.token_dump = true;
         cstreq("--ast-dump")    flags.ast_dump   = true;
         cstreq("--doc-gen")     flags.doc_gen    = true;
         
         cstreq("--help") {
            help();
            return 0;
         }

         cstreq("--version") {
            version();
            return 0;
         }
         
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

