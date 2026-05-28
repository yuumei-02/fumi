#include <mcu/core.h>
#include <mcu/containers.h>

#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "doc_gen.h"

void list_procedures(Ast* ast, AstNode* module, FILE* file) {
   fprintf(file,
      "<!DOCTYPE html>\n" \
      "<html lang=\"en\">\n" \
      "   <head>\n" \
      "      <meta charset=\"UTF-8\">\n" \
      "      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
      "      <title></title>\n" \
      "   </head>\n" \
      "   <body>\n");

   foreach (module->module.ANI_procedures, i) {
      ANI* node_i = Vector_get(&module->module.ANI_procedures, i);
      AstNode* procedure = Vector_get(&ast->AstNodes, *node_i);
      fprintf(file, "      <h1>%s</h1>\n", procedure->procedure.name.str_literal.chars);
   }

   fprintf(file,
      "   </body>\n" \
      "</html>\n");
}

i32 Ast_doc_gen(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   i32 result = mkdir("./docs", 0755);
   if (result && errno != EEXIST) {
      eprintln("[!] Failed to create docs directory, reason: \"%s\"", strerror(errno));
      return errno;
   }
   
   String path = String_from("./docs");
   foreach (self->AstNode_modules, i) {
      AstNode* module = Vector_get(&self->AstNode_modules, i);

      String_appendf(&path, "/%s.html", module->module.name.chars);
      FILE* file = fopen(path.chars, "wb");
      if (file == nullptr) {
         eprintln("[!] Failed to create file \"%s\", reason: \"%s\"", path.chars, strerror(errno));
         result = errno;
         goto cleanup;
      }

      list_procedures(self, module, file);

      if (fclose(file)) {
         eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", path.chars, strerror(errno));
         eprintln("[!] Continuing...");
      }
      
      String_clear(&path);
   }

   result = 0;

cleanup:
   String_free(&path);
   return result;
}

