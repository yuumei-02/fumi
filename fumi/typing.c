#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void create_symbol_tables_for_code_block(Vector* ANI_self, Ast* ast) {
   foreach ((*ANI_self), i) {
      ANI* ani = Vector_get(ANI_self, i);
      AstNode* node = Vector_get(&ast->AstNodes, *ani);

      switch (node->type) {
         case ANT_Module:    panic("unreachable");
         case ANT_Procedure: panic("unreachable");
         case ANT_Parameter: panic("unreachable");

         case ANT_VariableDecl:  continue;
         case ANT_ReturnStmt:    continue; 
         case ANT_BreakStmt:     continue;
         case ANT_ContinueStmt:  continue;
         case ANT_BinOp:         continue;
         case ANT_IntLiteral:    continue;
         case ANT_StringLiteral: continue;
         case ANT_Variable:      continue;
         case ANT_FunctionCall:  continue;

         case ANT_IfStmt: {
            node->if_stmt.scope = HashMap_new(Symbol)();
         } continue;

         case ANT_WhileStmt: {
            node->while_stmt.scope = HashMap_new(Symbol)();
         } continue;
      }

      panic("unreachable");
   }
}

void Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   foreach (self->AstNode_modules, i) {
      AstNode* module = Vector_get(&self->AstNode_modules, i);
      module->module.scope = HashMap_new(Symbol)();

      foreach (module->module.ANI_procedures, i) {
         ANI* proc_id = Vector_get(&module->module.ANI_procedures, i);
         AstNode* proc = Vector_get(&self->AstNodes, *proc_id);

         proc->procedure.scope = HashMap_new(Symbol)();
         create_symbol_tables_for_code_block(&proc->procedure.ANI_body, self);
      }
   }
}

