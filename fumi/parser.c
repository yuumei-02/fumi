#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"
#include "lexer.h"
#include "parser.h"
#include "reporter.h"

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

/// Returns a [Vector<ANI>]
Vector parse_code_block(Lexer* lexer, Ast* ast, ParseState* state) {
   Vector self = Vector_new(sizeof(ANI));

   loop {
      Token token = Lexer_next(lexer, &state->hard_failure);
      if (state->hard_failure) return self;

      switch (token.type) {
         /* case TT_Identifier: { */
         /* } break; */
      
         case TT_End: {
            return self;
         }

         case TT_Eof: {
            enter_panic();
            report_unexpected_token(lexer->path, token);
            return self;
         }

         default: {
            if (state->panic_mode) break;
            enter_panic();
            report_unexpected_token(lexer->path, token);
            Token_free(token);
         }
      }
   }

   return self;
}
   
ANI parse_procedure(Lexer* lexer, Ast* ast, ParseState* state) {
   AstNode self = {
      .type = ANT_Procedure,
      .procedure = {
         .name = String_dummy(),
         .return_type = String_from("void")
      }
   };

   TokenType expected[2] = { TT_Identifier };
   u32 expected_len = 1;

   loop {
      Token token = Lexer_next(lexer, &state->hard_failure);
      if (state->hard_failure) goto failure;

      bool found_expected = false;
      for (u32 i = 0; i < expected_len; ++i) {
         if (token.type == expected[i]) {
            found_expected = true;
            break;
         }
      }

      if (!found_expected) {
         enter_panic();
         if (expected_len > 1)
            report_unexpected_token(lexer->path, token);
         else
            report_unexpected_token_expected(lexer->path, token, expected[0]);
         Token_free(token);
         goto failure;
      }

      switch (token.type) {
         case TT_Identifier: {
            expected[0] = TT_Begin;
            self.procedure.name = token.str_literal;
         } break;

         case TT_Begin: {
            self.procedure.ANI_body = parse_code_block(lexer, ast, state);
            goto finish_parsing;
         } break;
      
         default: {
            panic("unreachable");
         }
      }
   }

finish_parsing:
   Vector_push(&ast->AstNodes, &self);
   return (ANI) (ast->AstNodes.length - 1);
   
failure:
   if (self.procedure.name.length > 0)
      String_free(&self.procedure.name);
      
   if (self.procedure.return_type.length > 0)
      String_free(&self.procedure.return_type);
   return -1;
}
   
void parse_module(const cstr path, Ast* ast, ParseState* state) {
   Lexer lexer = Lexer_new(path, &state->hard_failure);
   if (state->hard_failure) return;

   AstNode self = {
      .type = ANT_Module,
      .module = {
         .path = String_from((cstr) path),
         .ANI_procedures = Vector_new(sizeof(ANI))
      }
   };
   
   loop {
      Token token = Lexer_next(&lexer, &state->hard_failure);
      if (state->hard_failure) goto cleanup;

      switch (token.type) {
         case TT_Procedure: {
            exit_panic();
            ANI procedure = parse_procedure(&lexer, ast, state);
            if (procedure == -1) break;
            Vector_push(&self.module.ANI_procedures, &procedure);
         } break;
      
         case TT_Eof: {
            exit_panic();
            goto cleanup;
         }
         
         default: {
            Token_free(token);
            if (state->panic_mode) break;
            enter_panic();
            report_unexpected_token(lexer.path, token);
         }
      }
   }

cleanup:
   Vector_push(&ast->AstNode_modules, &self);
   Lexer_free(&lexer);
}

Ast Ast_parse_from_file_path(const cstr path, bool* failure) {
   mcu_assert(path != nullptr, "path can't be null");

   Ast self = {
      .AstNode_modules = Vector_new(sizeof(AstNode)),
      .AstNodes = Vector_new(sizeof(AstNode))
   };

   ParseState state = {0};
   parse_module(path, &self, &state);
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

         foreach (self->module.ANI_procedures, i) {
            ANI* node_i = Vector_get(&self->module.ANI_procedures, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_print(node, ast, indent + 1);
         }
      } return;
      
      case ANT_Procedure: {
         indprintln("Procedure");
         indprintln("├─name: %s", self->procedure.name.chars);
         indprintln("├─return-type: %s", self->procedure.return_type.chars);

         if (self->procedure.ANI_body.length <= 0) {
            indprintln("└─body: empty");
         } else {
            indprintln("└─body:");
         }

         foreach (self->procedure.ANI_body, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_print(node, ast, indent + 1);
         }
      } return;
      
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

