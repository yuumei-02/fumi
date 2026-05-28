#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void AstNode_create_symbol_table(AstNode* self, Ast* ast, Vector* scope_stack) {
   #define push_def_scope(scope) \
      scope = HashMap_new(Symbol)(); \
      HashMap(Symbol)* scope_ptr = &scope; \
      Vector_push(scope_stack, &scope_ptr)

   #define pop_scope() \
      Vector_pop(scope_stack)

   #define top_scope() \
      *(HashMap(Symbol)**) Vector_get(scope_stack, scope_stack->length - 1)

   switch (self->type) {
      case ANT_Module: {
         push_def_scope(self->module.scope);

         foreach (self->module.ANI_procedures, i) {
            ANI* proc_i = Vector_get(&self->module.ANI_procedures, i);
            AstNode* proc = Vector_get(&ast->AstNodes, *proc_i);
            
            HashMap_put(Symbol)(&self->module.scope, proc->procedure.name.chars, (Symbol) {
               .kind = SK_Proc,
               .procedure = *proc_i
            });
            AstNode_create_symbol_table(proc, ast, scope_stack);
         }

         pop_scope();
      } return;

      case ANT_Procedure: {
         push_def_scope(self->procedure.scope);

         foreach (self->procedure.ANI_parameters, i) {
            ANI* param_i = Vector_get(&self->procedure.ANI_parameters, i);
            AstNode* param = Vector_get(&ast->AstNodes, *param_i);

            HashMap_put(Symbol)(&self->procedure.scope, param->parameter.name.chars, (Symbol) {
               .kind = SK_Var
            });
         }

         foreach (self->procedure.ANI_body, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);

            AstNode_create_symbol_table(node, ast, scope_stack);
         }
         
         pop_scope();
      } return;

      case ANT_IfStmt: {
         push_def_scope(self->if_stmt.scope);

         if (self->if_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->if_stmt.expression);
            AstNode_create_symbol_table(expr, ast, scope_stack);
         }

         foreach (self->if_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->if_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_create_symbol_table(node, ast, scope_stack);
         }

         pop_scope();

         if (self->if_stmt.next_branch >= 0) {
            AstNode* next_branch = Vector_get(&ast->AstNodes, (usize) self->if_stmt.next_branch);
            AstNode_create_symbol_table(next_branch, ast, scope_stack);
         }

      } return;
      
      case ANT_WhileStmt: {
         push_def_scope(self->while_stmt.scope);

         if (self->while_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->while_stmt.expression);
            AstNode_create_symbol_table(expr, ast, scope_stack);
         }

         foreach (self->while_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->while_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_create_symbol_table(node, ast, scope_stack);
         }

         pop_scope();
      } return;

      case ANT_VariableDecl: {
         HashMap_put(Symbol)(top_scope(), self->variable_decl.name.chars, (Symbol) {
            .kind = SK_Var
         });

         if (self->variable_decl.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->variable_decl.expression);
            AstNode_create_symbol_table(expr, ast, scope_stack);
         }
      } return;
      
      case ANT_ReturnStmt: {
         if (self->return_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->return_stmt.expression);
            AstNode_create_symbol_table(expr, ast, scope_stack);
         }
      } return;
      
      case ANT_BinOp: {
         AstNode* left  = Vector_get(&ast->AstNodes, (usize) self->bin_op.left);
         AstNode* right = Vector_get(&ast->AstNodes, (usize) self->bin_op.right);
         
         AstNode_create_symbol_table(left,  ast, scope_stack);
         AstNode_create_symbol_table(right, ast, scope_stack);
      } return;
      
      case ANT_FunctionCall: {
         foreach (self->function_call.ANI_arguments, i) {
            ANI* arg_i = Vector_get(&self->function_call.ANI_arguments, i);
            AstNode* arg = Vector_get(&ast->AstNodes, *arg_i);

            AstNode_create_symbol_table(arg, ast, scope_stack);
         }
      } return;

      case ANT_Variable:      return;
      case ANT_IntLiteral:    return;
      case ANT_StringLiteral: return;
      case ANT_BreakStmt:     return;
      case ANT_ContinueStmt:  return;
      
      case ANT_Parameter: panic("unreachable");
   }

   panic("unreachable");
}

void Ast_create_global_scope(Ast* self, Vector* scope_stack) {
   self->global_scope = HashMap_new(Symbol)();
   Vector_push_create(scope_stack, (&self->global_scope));
   
   #define def_type(name, def) \
      HashMap_put(Symbol)(&self->global_scope, name, (Symbol) { \
         .kind = SK_Type, \
         .type = def \
      })

   def_type("isize", ((Type) { .kind = TK_Int, .integer = { .bits = Bit64, .is_signed = true }}));
   def_type("i64",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit64, .is_signed = true }}));
   def_type("i32",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit32, .is_signed = true }}));
   def_type("i16",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit16, .is_signed = true }}));
   def_type("i8",    ((Type) { .kind = TK_Int, .integer = { .bits = Bit8,  .is_signed = true }}));

   def_type("usize", ((Type) { .kind = TK_Int, .integer = { .bits = Bit64, .is_signed = false }}));
   def_type("u64",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit64, .is_signed = false }}));
   def_type("u32",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit32, .is_signed = false }}));
   def_type("u16",   ((Type) { .kind = TK_Int, .integer = { .bits = Bit16, .is_signed = false }}));
   def_type("u8",    ((Type) { .kind = TK_Int, .integer = { .bits = Bit8,  .is_signed = false }}));

   def_type("void", ((Type) { .kind = TK_Void }));
}

void Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");
   
   Vector scope_stack = Vector_new(sizeof(HashMap(Symbol)*));
   Ast_create_global_scope(self, &scope_stack);

   foreach (self->AstNode_modules, i) {
      AstNode* module = Vector_get(&self->AstNode_modules, i);
      AstNode_create_symbol_table(module, self, &scope_stack);
   }
}

