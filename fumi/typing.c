#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void Ast_symbol_visitor(AstNode* node, nullable void* opt) {
   static Vector symbol_stack;
   if (symbol_stack.capacity <= 0)
      symbol_stack = Vector_new(sizeof(HashMap(Symbol)*));

   #define push_symbol_stack(symbol_table_ptr) \
      HashMap(Symbol)* sym = symbol_table_ptr; \
      Vector_push(&symbol_stack, &sym)

   #define symbol_stack_at(index) \
      *(HashMap(Symbol)**) Vector_get(&symbol_stack, index)

   #define push_symbol_to_stack_at(index, key, symbol) \
      HashMap_put(Symbol)(symbol_stack_at(index), key, symbol)

   #define top_scope \
      (symbol_stack.length - 1)

   switch (node->type) {
      case ANT_Module: {
         node->module.scope = HashMap_new(Symbol)();
         push_symbol_stack(&node->module.scope);
      } return;

      case ANT_Procedure: {
         node->procedure.scope = HashMap_new(Symbol)();
         push_symbol_stack(&node->procedure.scope);

         push_symbol_to_stack_at(0, node->procedure.name.chars, (Symbol) {
            .kind = SK_Proc
         });
      } return;

      case ANT_Parameter: {
         push_symbol_to_stack_at(top_scope, node->parameter.name.chars, (Symbol) {
            .kind = SK_Var
         });
      } return;
      
      case ANT_VariableDecl: {
         push_symbol_to_stack_at(top_scope, node->variable_decl.name.chars, (Symbol) {
            .kind = SK_Var
         });
      } return;
      
      case ANT_IfStmt: {
         node->if_stmt.scope = HashMap_new(Symbol)();
         push_symbol_stack(&node->if_stmt.scope);
      } return;

      case ANT_WhileStmt: {
         node->while_stmt.scope = HashMap_new(Symbol)();
         push_symbol_stack(&node->while_stmt.scope);
      } return;

      case ANT_IntLiteral:    return;
      case ANT_StringLiteral: return;
      case ANT_ReturnStmt:    return;
      case ANT_BreakStmt:     return;
      case ANT_ContinueStmt:  return;
      case ANT_BinOp:         return; 
      case ANT_Variable:      return;
      case ANT_FunctionCall:  return;
   }

   panic("unreachable");
}

void Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   Ast_walk(self, &Ast_symbol_visitor, nullptr);
}

