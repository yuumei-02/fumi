#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void AstNode_create_symbol_table(AstNode* self, ANI self_index, Ast* ast, Vector* scope_stack, bool* invalid_program) {
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
               .procedure = {
                  .node = *proc_i
               }
            });
            AstNode_create_symbol_table(proc, *proc_i, ast, scope_stack, invalid_program);
         }

         pop_scope();
      } return;

      case ANT_Procedure: {
         push_def_scope(self->procedure.scope);

         foreach (self->procedure.ANI_parameters, i) {
            ANI* param_i = Vector_get(&self->procedure.ANI_parameters, i);
            AstNode* param = Vector_get(&ast->AstNodes, *param_i);

            HashMap_put(Symbol)(&self->procedure.scope, param->parameter.name.chars, (Symbol) {
               .kind = SK_Var,
               .var = {
                  .node = *param_i
               }
            });
         }

         foreach (self->procedure.ANI_body, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);

            AstNode_create_symbol_table(node, *node_i, ast, scope_stack, invalid_program);
         }
         
         pop_scope();
      } return;

      case ANT_IfStmt: {
         push_def_scope(self->if_stmt.scope);

         if (self->if_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->if_stmt.expression);
            AstNode_create_symbol_table(expr, self->if_stmt.expression, ast, scope_stack, invalid_program);
         }

         foreach (self->if_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->if_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_create_symbol_table(node, *node_i, ast, scope_stack, invalid_program);
         }

         pop_scope();

         if (self->if_stmt.next_branch >= 0) {
            AstNode* next_branch = Vector_get(&ast->AstNodes, (usize) self->if_stmt.next_branch);
            AstNode_create_symbol_table(next_branch, self->if_stmt.next_branch, ast, scope_stack, invalid_program);
         }

      } return;
      
      case ANT_WhileStmt: {
         push_def_scope(self->while_stmt.scope);

         if (self->while_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->while_stmt.expression);
            AstNode_create_symbol_table(expr, self->while_stmt.expression, ast, scope_stack, invalid_program);
         }

         foreach (self->while_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->while_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_create_symbol_table(node, *node_i, ast, scope_stack, invalid_program);
         }

         pop_scope();
      } return;

      case ANT_VariableDecl: {
         Symbol* existing_decl = HashMap_get(Symbol)(top_scope(), self->variable_decl.name.chars);
         if (existing_decl != nullptr) {
            eprintln("Redeclaration of variable \"%s\"", self->variable_decl.name.chars);
            *invalid_program = true;
            return;
         }
      
         HashMap_put(Symbol)(top_scope(), self->variable_decl.name.chars, (Symbol) {
            .kind = SK_Var,
            .var = {
               .node = self_index
            }
         });

         if (self->variable_decl.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->variable_decl.expression);
            AstNode_create_symbol_table(expr, self->variable_decl.expression, ast, scope_stack, invalid_program);
         }
      } return;
      
      case ANT_ReturnStmt: {
         if (self->return_stmt.expression >= 0) {
            AstNode* expr = Vector_get(&ast->AstNodes, (usize) self->return_stmt.expression);
            AstNode_create_symbol_table(expr, self->return_stmt.expression, ast, scope_stack, invalid_program);
         }
      } return;
      
      case ANT_BinOp: {
         AstNode* left  = Vector_get(&ast->AstNodes, (usize) self->bin_op.left);
         AstNode* right = Vector_get(&ast->AstNodes, (usize) self->bin_op.right);
         
         AstNode_create_symbol_table(left, self->bin_op.left, ast, scope_stack, invalid_program);
         AstNode_create_symbol_table(right, self->bin_op.right, ast, scope_stack, invalid_program);
      } return;
      
      case ANT_FunctionCall: {
         foreach (self->function_call.ANI_arguments, i) {
            ANI* arg_i = Vector_get(&self->function_call.ANI_arguments, i);
            AstNode* arg = Vector_get(&ast->AstNodes, *arg_i);

            AstNode_create_symbol_table(arg, *arg_i, ast, scope_stack, invalid_program);
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

bool Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");
   
   Vector scope_stack = Vector_new(sizeof(HashMap(Symbol)*));
   Ast_create_global_scope(self, &scope_stack);
   bool invalid_program = false;

   foreach (self->AstNode_modules, i) {
      AstNode* module = Vector_get(&self->AstNode_modules, i);
      AstNode_create_symbol_table(module, (ANI) i, self, &scope_stack, &invalid_program);
   }

   Vector_free(&scope_stack);
   return invalid_program;
}

typedef struct {
   bool finished;
   bool valid_program;
   Vector scope_stack;
   Ast* ast;
} AnalysisState;

void type_check_variable(AstNode* variable, AnalysisState* state) {
   if (variable->status != ANS_Unchecked) return;

   bool found = false;

   if (state->scope_stack.length > 0) {
      usize i = state->scope_stack.length - 1;
      loop {
         SymbolTable** scope = Vector_get(&state->scope_stack, i);
         Symbol* symbol = HashMap_get(Symbol)(*scope, variable->variable.chars);
         if (symbol != nullptr) {
            if (symbol->kind != SK_Var) {
               eprintln("expected symbol \"%s\" to be a variable, got %s", variable->variable.chars, SymbolKind_to_cstr(symbol->kind));
               state->valid_program = false;
               return;
            }
            found = true;
            break;
         }

         if (i == 0)
            break;
         else
            i--;
      }
   }

   if (!found) {
      eprintln("undefined variable \"%s\"", variable->variable.chars);
      variable->status = ANS_Poisen;
      state->valid_program = false;
   } else {
      variable->status = ANS_Valid;
   }
}

void Ast_semantic_walker(AstNode* node, bool exited, nullable void* opt) {
   mcu_assert(opt != nullptr, "opt can't be null");
   
   AnalysisState* state = opt;

   switch (node->type) {
      case ANT_Module: {
         if (exited)
            Vector_pop(&state->scope_stack);
         else
            Vector_push_create(&state->scope_stack, &node->module.scope);
      } break;
      
      case ANT_Procedure: {
         if (exited)
            Vector_pop(&state->scope_stack);
         else
            Vector_push_create(&state->scope_stack, &node->procedure.scope);
      } break;

      case ANT_Parameter:     break;
      case ANT_VariableDecl:  break;
      case ANT_ReturnStmt:    break;
      
      case ANT_IfStmt: {
         if (exited)
            Vector_pop(&state->scope_stack);
         else
            Vector_push_create(&state->scope_stack, &node->if_stmt.scope);
      } break;
      
      case ANT_WhileStmt: {
         if (exited)
            Vector_pop(&state->scope_stack);
         else
            Vector_push_create(&state->scope_stack, &node->while_stmt.scope);
      } break;
      
      case ANT_BreakStmt:     break;
      case ANT_ContinueStmt:  break;
      case ANT_BinOp:         break;
      case ANT_IntLiteral:    break;
      case ANT_StringLiteral: break;
      
      case ANT_Variable: {
         type_check_variable(node, state);
      } break;
      
      case ANT_FunctionCall: break;
   }
}

bool Ast_analyize_semantics(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   AnalysisState state = {
      .scope_stack = Vector_new(sizeof(SymbolTable*)),
      .valid_program = true,
      .ast = self
   };

   SymbolTable* global_scope = &self->global_scope;
   Vector_push(&state.scope_stack, &global_scope);

   Ast_walk(self, &Ast_semantic_walker, &state);
   
   Vector_free(&state.scope_stack);
   return !state.valid_program;
}

