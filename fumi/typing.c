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
            
            HashMap_put(Symbol)(&self->module.scope, proc->procedure.name.str_literal.chars, (Symbol) {
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

            HashMap_put(Symbol)(&self->procedure.scope, param->parameter.name.str_literal.chars, (Symbol) {
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
         Symbol* existing_decl = HashMap_get(Symbol)(top_scope(), self->variable_decl.name.str_literal.chars);
         if (existing_decl != nullptr) {
            println("%s:%zu:%zu: error: Redeclaration of variable \"%s\"",
               self->path,
               self->y,
               self->x,
               self->variable_decl.name.str_literal.chars);
            *invalid_program = true;
            return;
         }
      
         HashMap_put(Symbol)(top_scope(), self->variable_decl.name.str_literal.chars, (Symbol) {
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

Symbol* find_symbol(Vector* scope_stack, cstr symbol_name) {
   if (scope_stack->length < 1)
      return nullptr;

   usize i = scope_stack->length - 1;
   loop {
      SymbolTable** scope = Vector_get(scope_stack, i);
      Symbol* symbol = HashMap_get(Symbol)(*scope, symbol_name);
      if (symbol != nullptr)
         return symbol;
   
      if (i == 0) return nullptr;
      i--;
   }
}

/// Returns the str representation of the type of the variable if found.
/// If the variable was not found or is not a variable, [null] gets returned.
cstr type_check_variable(AstNode* variable, AnalysisState* state) {
   bool found = false;
   AstNode* decl;

   if (state->scope_stack.length > 0) {
      usize i = state->scope_stack.length - 1;
      loop {
         SymbolTable** scope = Vector_get(&state->scope_stack, i);
         Symbol* symbol = HashMap_get(Symbol)(*scope, variable->variable.chars);
         if (symbol != nullptr) {
            if (symbol->kind != SK_Var) {
               eprintln("%s:%zu:%zu: error: expected symbol \"%s\" to be a variable, got %s",
                  variable->path, variable->y, variable->x,
                  variable->variable.chars, SymbolKind_to_cstr(symbol->kind));
               state->valid_program = false;
               variable->status = ANS_Poison;
               return nullptr;
            }

            decl = Vector_get(&state->ast->AstNodes, symbol->var.node);
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
      eprintln("%s:%zu:%zu: error: undefined variable \"%s\"",
         variable->path, variable->y, variable->x,
         variable->variable.chars);
      variable->status = ANS_Poison;
      state->valid_program = false;
      return nullptr;
   } else {
      variable->status = ANS_Valid;
      return decl->variable_decl.type.str_literal.chars;
   }
}

/// returns a [cstr] of the type of the expression.
/// The return value may be [null] when no type could be found or determined.
/// Check the root expression's status to see the result of the type check.
cstr type_check_expression(AstNode* expression, AnalysisState* state) {
   switch (expression->type) {
      case ANT_BinOp: {
         AstNode* left = Vector_get(&state->ast->AstNodes, expression->bin_op.left);
         AstNode* right = Vector_get(&state->ast->AstNodes, expression->bin_op.right);

         cstr left_type = type_check_expression(left, state);
         cstr right_type = type_check_expression(right, state);
         if (left_type == nullptr) {
            if (right_type == nullptr) {
               expression->status = ANS_Poison;
               return nullptr;
            }
            expression->status = ANS_Valid;
            return right_type;
         }

         if (right_type == nullptr) {
            expression->status = ANS_Valid;
            return left_type;
         }

         if (strcmp(left_type, right_type) != 0) {
            state->valid_program = false;
            expression->status = ANS_Poison;
            println("%s:%zu:%zu: error: incompatible datatype for operation: \"%s\" %s \"%s\"",
               left->path, left->y, left->x,
               left_type, Operator_to_cstr(expression->bin_op.operator), right_type);
            return left_type;
         }

         expression->status = ANS_Valid;
         return left_type;
      } break;
      
      case ANT_IntLiteral: {
         expression->status = ANS_Valid;
         return "i32";
      }
      
      case ANT_Variable: {
         return type_check_variable(expression, state);
      }
      
      case ANT_StringLiteral: {
         expression->status = ANS_Valid;
         return "char*";
      }

      case ANT_FunctionCall: {
         Symbol* proc_sym = find_symbol(&state->scope_stack, expression->function_call.function.chars);
         if (proc_sym == nullptr) {
            state->valid_program = false;
            expression->status = ANS_Poison;
            eprintln("%s:%zu:%zu: error: procedure \"%s\" does not exist and may be out of scope.",
               expression->path, expression->y, expression->x, expression->function_call.function.chars);
            return nullptr;
         }

         if (proc_sym->kind != SK_Proc) {
            state->valid_program = false;
            expression->status = ANS_Poison;
            eprintln("%s:%zu:%zu: error: call to \"%s\" is not a procedure does.",
               expression->path, expression->y, expression->x, expression->function_call.function.chars);
            return nullptr;
         }

         AstNode* proc_def = Vector_get(&state->ast->AstNodes, proc_sym->procedure.node);
         if (proc_def->procedure.ANI_parameters.length != expression->function_call.ANI_arguments.length) {
            state->valid_program = false;
            expression->status = ANS_Poison;
            eprintln("%s:%zu:%zu: error: call to procedure \"%s\" does not match the definition's argument count",
               expression->path, expression->y, expression->x, expression->procedure.name.str_literal.chars);
         }
         
         foreach (expression->function_call.ANI_arguments, i) {
            if (i >= proc_def->procedure.ANI_parameters.length) break;

            ANI* param_i = Vector_get(&proc_def->procedure.ANI_parameters, i);
            ANI* arg_i = Vector_get(&expression->function_call.ANI_arguments, i);
            
            AstNode* param = Vector_get(&state->ast->AstNodes, *param_i);
            AstNode* arg = Vector_get(&state->ast->AstNodes, *arg_i);

            cstr arg_type = type_check_expression(arg, state);
            if (arg_type == nullptr || arg->status == ANS_Poison)
               continue;

            if (strcmp(arg_type, param->parameter.type.str_literal.chars) != 0) {
               state->valid_program = false;
               expression->status = ANS_Poison;
               eprintln("%s:%zu:%zu: error: argument in procedure call to \"%s\" does not match the definition's parameter type of \"%s\"",
                  arg->path, arg->y, arg->x,
                  expression->function_call.function.chars,
                  param->parameter.type.str_literal.chars);
            }
         }

         if (expression->status == ANS_Unchecked) {
            expression->status = ANS_Valid;
         }

         return proc_def->procedure.return_type.str_literal.chars;
      }

      default: {
         panic("unreachable");
      }
   }

   return nullptr;
}

void type_check_variable_decl(AstNode* variable_decl, AnalysisState* state) {
   if (variable_decl->status != ANS_Unchecked) return;

   if (strcmp("@infer", variable_decl->variable_decl.type.str_literal.chars) == 0) {
      if (variable_decl->variable_decl.expression < 0) {
         state->valid_program = false;
         variable_decl->status = ANS_Poison;
         eprintln("%s:%zu:%zu: error: unable to infer the variable's type from the expression",
            variable_decl->path, variable_decl->y, variable_decl->x);
         return;
      }

      AstNode* expression = Vector_get(&state->ast->AstNodes, variable_decl->variable_decl.expression);
      cstr expr_type = type_check_expression(expression, state);
      if (expr_type == nullptr || expression->status != ANS_Valid) {
         state->valid_program = false;
         variable_decl->status = ANS_Poison;
         eprintln("%s:%zu:%zu: error: unable to infer the variable's type from the expression",
            expression->path, expression->y, expression->x);
         return;
      }

      String_free(&variable_decl->variable_decl.type.str_literal);
      variable_decl->variable_decl.type.str_literal = String_from(expr_type);
   } else {
      if (variable_decl->variable_decl.expression < 0) {
         variable_decl->status = ANS_Valid;
         return;
      }
   
      AstNode* expression = Vector_get(&state->ast->AstNodes, variable_decl->variable_decl.expression);
      cstr expr_type = type_check_expression(expression, state);
      if (expr_type == nullptr || expression->status != ANS_Valid) {
         variable_decl->status = ANS_Valid;
         return;
      }

      if (strcmp(variable_decl->variable_decl.type.str_literal.chars, expr_type) != 0) {
         state->valid_program = false;
         variable_decl->status = ANS_Poison;
         eprintln("%s:%zu:%zu: error: the expression of the variable decleration does not match the specified type of \"%s\"",
            expression->path, expression->y, expression->x,
            variable_decl->variable_decl.type.str_literal.chars);
         return;
      }
   }

   variable_decl->status = ANS_Valid;
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

      case ANT_Parameter: break;
      
      case ANT_VariableDecl: {
         type_check_variable_decl(node, state);
      } break;
      
      case ANT_ReturnStmt: break;
      
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
      
      case ANT_BreakStmt:    break;
      case ANT_ContinueStmt: break;
      
      case ANT_BinOp: {
         if (node->status == ANS_Unchecked)
            type_check_expression(node, state);
      } break;
      
      case ANT_IntLiteral:    break;
      case ANT_StringLiteral: break;

      case ANT_Variable: {
         if (node->status == ANS_Unchecked)
            type_check_variable(node, state);
      } break;
      
      case ANT_FunctionCall: {
         if (node->status == ANS_Unchecked)
            type_check_expression(node, state);
      } break;
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

