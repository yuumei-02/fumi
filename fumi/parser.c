#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
   bool panic_mode;
   bool failure;
   bool hard_failure;
} ParseState;

#define enter_panic() \
   state->panic_mode = true; \
   state->failure = true;

#define exit_panic() \
   state->panic_mode = false;

void parse_module(const cstr path, Ast* ast, ParseState* state, CompileFlags* flags) {
   Lexer lexer = Lexer_new(path, &state->hard_failure);
   if (state->hard_failure) return;

   AstNode self = {
      .type = ANT_Module,
      .module = {
         .path = String_from((cstr) path),
         .ANI_procedures = Vector_new(sizeof(ANI))
      }
   };

   Vector_push(&ast->AstNode_modules, &self);
   Lexer_free(&lexer);
}

Ast Ast_parse_from_file_path(const cstr path, CompileFlags flags, bool* failure) {
   mcu_assert(path != nullptr, "path can't be null");

   Ast self = {
      .AstNode_modules = Vector_new(sizeof(AstNode)),
      .AstNodes = Vector_new(sizeof(AstNode))
   };

   ParseState state = {0};
   parse_module(path, &self, &state, &flags);
   if (state.hard_failure || state.failure)
      goto failure;

   if (failure != nullptr) *failure = false;
   return self;

failure:
   if (failure != nullptr) *failure = true;
   return self;
}

void Ast_free(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   foreach (self->AstNode_modules, i) {
      AstNode* node = Vector_get(&self->AstNode_modules, i);
      String_free(&node->module.path);
      Vector_free(&node->module.ANI_procedures);
   }

   foreach (self->AstNodes, i) {
      AstNode* node = Vector_get(&self->AstNodes, i);

      switch (node->type) {
         case ANT_Procedure: {
            String_free(&node->procedure.name);
            String_free(&node->procedure.return_type);
            Vector_free(&node->procedure.ANI_body);
         } continue;

         case ANT_VariableDecl: {
            String_free(&node->variable_decl.name);
            String_free(&node->variable_decl.type);
         } continue;

         case ANT_IntLiteral: break;
         case ANT_Module: {
            panic("unreachable");
         }
      }

      panic("unreachable");
   }
   
   Vector_free(&self->AstNode_modules);
   Vector_free(&self->AstNodes);
   *self = (Ast) {0};
}

// └├│─
void AstNode_print(AstNode* self, Ast* ast, i32 indent) {
   if (self == nullptr) return;

   #define indprintln(format, ...) \
      printf("%*c", indent * 3, ' '); \
      println(format __VA_OPT__(,) __VA_ARGS__)

   switch (self->type) {
      case ANT_Module: {
         indprintln("Module");
         indprintln("├─path: %s", self->module.path.chars);
         
         if (self->module.ANI_procedures.length <= 0) {
            indprintln("└─body: empty");
         } else {
            indprintln("└─body:");
         }
      } return;
      
      case ANT_Procedure:    mcu_todo("not yet implemented"); return;
      case ANT_VariableDecl: mcu_todo("not yet implemented"); return;
      case ANT_IntLiteral:   mcu_todo("not yet implemented"); return;
   }

   panic("unreachable");
}

void Ast_print(Ast self) {
   foreach (self.AstNode_modules, i) {
      AstNode* node = Vector_get(&self.AstNode_modules, i);
      AstNode_print(node, &self, 0);
   }
}

