#include <mcu/core.h>

#include "ast.h"

const cstr SymbolKind_to_cstr(SymbolKind self) {
   switch (self) {
      case SK_Proc:  return "Proc";
      case SK_Type:  return "Type";
      case SK_Var:   return "Var";
   }

   return "Unknown";
}

Operator TokenType_to_operator(TokenType type, nullable bool* is_operator) {
   if (is_operator != nullptr) *is_operator = true;

   switch (type) {
      case TT_Plus: return O_Add;
      case TT_Min:  return O_Sub;
      case TT_Mul:  return O_Mul;
      case TT_Div:  return O_Div;

      case TT_DoubleEquals: return O_Is;
      case TT_NotEquals:    return O_IsNot;
      case TT_Less:         return O_Less;
      case TT_Great:        return O_Great;
      case TT_LessEquals:   return O_LessEqu;
      case TT_GreatEquals:  return O_GreatEqu;
      case TT_DoubleAnd:    return O_And;
      case TT_DoublePipe:   return O_Or;

      case TT_Equals:     return O_Equ;
      case TT_PlusEquals: return O_PlusEqu;
      case TT_MinEquals:  return O_MinEqu;
      case TT_MulEquals:  return O_MulEqu;
      case TT_DivEquals:  return O_DivEqu;
      
      default: {
         if (is_operator != nullptr) *is_operator = false;
         return -1;
      }
   }
}

const cstr Operator_to_cstr(Operator operator) {
   switch (operator) {
      case O_Add: return "Add";
      case O_Sub: return "Sub";
      case O_Mul: return "Mul";
      case O_Div: return "Div";

      case O_Less:     return "Less";
      case O_Great:    return "Great";
      case O_LessEqu:  return "LessEqu";
      case O_GreatEqu: return "GreatEqu";
      case O_Is:       return "Is";
      case O_IsNot:    return "IsNot";
      case O_And:      return "And";
      case O_Or:       return "Or";

      case O_Equ:     return "Equ";
      case O_PlusEqu: return "PlusEqu";
      case O_MinEqu:  return "MinEqu";
      case O_MulEqu:  return "MulEqu";
      case O_DivEqu:  return "DivEqu";
   }

   return "Unknown";
}

// @reference: https://en.cppreference.com/c/language/operator_precedence
isize Operator_get_precedence(Operator operator) {
   switch (operator) {
      case O_Mul:       return 7;
      case O_Div:       return 7;
      
      case O_Add:       return 6;
      case O_Sub:       return 6;
      
      case O_Less:      return 5;
      case O_Great:     return 5;
      case O_LessEqu:   return 5;
      case O_GreatEqu:  return 5;
      
      case O_Is:        return 4;
      case O_IsNot:     return 4;
      
      case O_And:       return 3;
      case O_Or:        return 2;
      
      case O_Equ:       return 1;
      case O_PlusEqu:   return 1;
      case O_MinEqu:    return 1;
      case O_MulEqu:    return 1;
      case O_DivEqu:    return 1;
   }
   
   panic("unreachable");
}

OperatorAssociation Operator_get_association(Operator self) {
   switch (self) {
      case O_Add: return OA_Left;
      case O_Sub: return OA_Left;
      case O_Mul: return OA_Left;
      case O_Div: return OA_Left;

      case O_Is:        return OA_Left;
      case O_IsNot:     return OA_Left;
      case O_And:       return OA_Left;
      case O_Or:        return OA_Left;
      case O_Less:      return OA_Left;
      case O_Great:     return OA_Left;
      case O_LessEqu:   return OA_Left;
      case O_GreatEqu:  return OA_Left;

      case O_Equ:     return OA_Right;
      case O_PlusEqu: return OA_Right;
      case O_MinEqu:  return OA_Right;
      case O_MulEqu:  return OA_Right;
      case O_DivEqu:  return OA_Right;
   }
   
   unreachable();
}

void Ast_free(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   foreach (self->AstNode_modules, i) {
      AstNode* node = Vector_get(&self->AstNode_modules, i);
      String_free(&node->module.path);
      String_free(&node->module.name);
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

         case ANT_Parameter: {
            String_free(&node->parameter.name);
            String_free(&node->parameter.type);
         } continue;

         case ANT_VariableDecl: {
            String_free(&node->variable_decl.name);
            String_free(&node->variable_decl.type);
         } continue;

         case ANT_Variable: {
            String_free(&node->variable);
         } continue;

         case ANT_IfStmt: {
            Vector_free(&node->if_stmt.ANI_body);
         } continue;

         case ANT_WhileStmt: {
            Vector_free(&node->while_stmt.ANI_body);
         } continue;

         case ANT_StringLiteral: {
            String_free(&node->str_literal);
         } continue;

         case ANT_FunctionCall: {
            String_free(&node->function_call.function);
            Vector_free(&node->function_call.ANI_arguments);
         } continue;

         case ANT_BreakStmt:    continue;
         case ANT_ContinueStmt: continue;
         case ANT_ReturnStmt:   continue;
         case ANT_BinOp:        continue;
         case ANT_IntLiteral:   continue;
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

void AstNode_print(AstNode* self, Ast* ast, i32 indent) {
   if (self == nullptr) return;

   #define indprintln(format, ...) \
      for (i32 i = 0; i < indent; ++i) \
         printf("│  "); \
      println(format __VA_OPT__(,) __VA_ARGS__)

   switch (self->type) {
      case ANT_Module: {
         indprintln("Module");
         indprintln("├─path: %s", self->module.path.chars);
         indprintln("├─name: %s", self->module.name.chars);
         
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
         if (self->procedure.ANI_parameters.length <= 0) {
            indprintln("├─parameters: empty");
         } else {
            indprintln("├─parameters:");
         }

         foreach (self->procedure.ANI_parameters, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_parameters, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_print(node, ast, indent + 1);
         }

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

      case ANT_Parameter: {
         indprintln("Parameter");
         indprintln("├─name: %s", self->parameter.name.chars);
         indprintln("└─type: %s", self->parameter.type.chars);
      } return;
      
      case ANT_VariableDecl: {
         indprintln("VariableDecl");
         indprintln("├─name: %s", self->variable_decl.name.chars);
         indprintln("├─type: %s", self->variable_decl.type.chars);
         
         if (self->variable_decl.expression == -1) {
            indprintln("└─expression: empty");
            return;
         }
         indprintln("└─expression:");

         AstNode* node = Vector_get(&ast->AstNodes, self->variable_decl.expression);
         AstNode_print(node, ast, indent + 1);
      } return;

      case ANT_IfStmt: {
         indprintln("IfStmt");
         if (self->if_stmt.expression < 0) {
            indprintln("├─expression: empty");
         } else {
            indprintln("├─expression:");
            AstNode* node = Vector_get(&ast->AstNodes, self->if_stmt.expression);
            AstNode_print(node, ast, indent + 1);
         }
         
         if (self->if_stmt.ANI_body.length <= 0) {
            indprintln("├─body: empty");
         } else {
            indprintln("├─body:");
            foreach (self->if_stmt.ANI_body, i) {
               ANI* node_i = Vector_get(&self->if_stmt.ANI_body, i);
               AstNode* node = Vector_get(&ast->AstNodes, *node_i);
               AstNode_print(node, ast, indent + 1);
            }
         }

         if (self->if_stmt.next_branch < 0) {
            indprintln("└─next_branch: empty");
         } else {
            indprintln("└─next_branch:");
            AstNode* node = Vector_get(&ast->AstNodes, self->if_stmt.next_branch);
            AstNode_print(node, ast, indent + 1);
         }
      } return;

      case ANT_WhileStmt: {
         indprintln("WhileStmt");
         indprintln("├─expression:");
         
         mcu_assert(self->while_stmt.expression != -1,
            "While statements must always have an expression");
         AstNode* node = Vector_get(&ast->AstNodes, self->while_stmt.expression);
         AstNode_print(node, ast, indent + 1);
         
         if (self->while_stmt.ANI_body.length <= 0) {
            indprintln("└─body: empty");
         } else {
            indprintln("└─body:");
            foreach (self->while_stmt.ANI_body, i) {
               ANI* node_i = Vector_get(&self->while_stmt.ANI_body, i);
               AstNode* node = Vector_get(&ast->AstNodes, *node_i);
               AstNode_print(node, ast, indent + 1);
            }
         }
      } return;

      case ANT_BreakStmt: {
         indprintln("BreakStmt");
      } return;

      case ANT_ContinueStmt: {
         indprintln("ContinueStmt");
      } return;

      case ANT_ReturnStmt: {
         indprintln("ReturnStmt");
         if (self->return_stmt.expression == -1) {
            indprintln("└─expression: empty");
            return;
         }
         
         indprintln("└─expression:");
         AstNode* node = Vector_get(&ast->AstNodes, self->return_stmt.expression);
         AstNode_print(node, ast, indent + 1);
      } return;

      case ANT_BinOp: {
         indprintln("BinOp");
         indprintln("├─operator: %s", Operator_to_cstr(self->bin_op.operator));
         indprintln("├─left:");
         AstNode* node = Vector_get(&ast->AstNodes, self->bin_op.left);
         AstNode_print(node, ast, indent + 1);
         indprintln("└─right:");
         node = Vector_get(&ast->AstNodes, self->bin_op.right);
         AstNode_print(node, ast, indent + 1);
      } return;
      
      case ANT_IntLiteral: {
         indprintln("%ld", self->int_literal);
      } return;

      case ANT_StringLiteral: {
         indprintln("\"%s\"", self->str_literal.chars);
      } return;

      case ANT_Variable: {
         indprintln("Variable");
         indprintln("└─name: %s", self->variable.chars);
      } return;

      case ANT_FunctionCall: {
         indprintln("FunctionCall");
         indprintln("├─function: %s", self->function_call.function.chars);

         if (self->function_call.ANI_arguments.length <= 0) {
            indprintln("└─arguments: empty");
         } else {
            indprintln("└─arguments:");
         }

         foreach (self->function_call.ANI_arguments, i) {
            ANI* node_i = Vector_get(&self->function_call.ANI_arguments, i);
            AstNode* argument = Vector_get(&ast->AstNodes, *node_i);
            AstNode_print(argument, ast, indent + 1);
         }
      } return;
   }

   panic("unreachable");
}

static void __print_symbol_table(cstr key, Symbol* symbol, void* data) {
   mcu_assert(data != nullptr, "data can't be null");
   i32 indent = *(i32*) data;

   #define indprintln(format, ...) \
      for (i32 i = 0; i < indent; ++i) \
         printf("│  "); \
      println(format __VA_OPT__(,) __VA_ARGS__)

   indprintln("├─(symbol: %s, kind: %s)", key, SymbolKind_to_cstr(symbol->kind));
}

typedef struct {
   i32 indent;
   Ast* ast;
} AstPrintState;

void AstNode_print_symbol_table(AstNode* self, void* opt) {
   AstPrintState* state = opt;
   #undef indprintln
   #define indprintln(format, ...) \
      for (i32 i = 0; i < state->indent; ++i) \
         printf("│  "); \
      println(format __VA_OPT__(,) __VA_ARGS__)

   #define output_symbol_table(scope) \
      if (scope.length <= 0) { \
         indprintln("└─symbol table: empty"); \
      } else { \
         indprintln("└─symbol table:"); \
         i32 new_indent = ++state->indent; \
         HashMap_foreach(Symbol)(&scope, &__print_symbol_table, &new_indent); \
         indprintln("└─end"); \
         state->indent--; \
      }

   switch (self->type) {
      case ANT_Module: {
         indprintln("Module : %s", self->module.path.chars);
         output_symbol_table(self->module.scope);
      } return;
      
      case ANT_Procedure: {
         indprintln("Procedure : %s", self->procedure.name.chars);
         output_symbol_table(self->procedure.scope);
      } return;

      case ANT_IfStmt: {
         indprintln("If statement");
         output_symbol_table(self->if_stmt.scope);
      } return;

      case ANT_WhileStmt: {
         indprintln("While stmt");
         output_symbol_table(self->while_stmt.scope);
      } return;
      
      case ANT_Parameter:     return;
      case ANT_VariableDecl:  return;
      case ANT_ReturnStmt:    return;
      case ANT_BreakStmt:     return;
      case ANT_ContinueStmt:  return;
      case ANT_BinOp:         return;
      case ANT_IntLiteral:    return;
      case ANT_StringLiteral: return;
      case ANT_Variable:      return;
      case ANT_FunctionCall:  return;
   }

   panic("unreachable");
}

void Ast_print(Ast self) {
   foreach (self.AstNode_modules, i) {
      AstNode* node = Vector_get(&self.AstNode_modules, i);
      AstNode_print(node, &self, 0);
   }

   println("Ast : Global scope");
   if (self.global_scope.length <= 0) {
      println("└─symbol table: empty");
   } else {
      println("└─symbol table:");
      i32 indent = 1;
      HashMap_foreach(Symbol)(&self.global_scope, &__print_symbol_table, &indent);
      println("└─end");
   }
   
   AstPrintState state = { .indent = 0, .ast = &self }; 
   Ast_walk(&self, &AstNode_print_symbol_table, &state);
}

void AstNode_visit(AstNode* self, Ast* ast, AstWalker walker, nullable void* opt) {
   switch (self->type) {
      case ANT_Module: {
         walker(self, opt);
         if (self->module.ANI_procedures.length <= 0) return;

         foreach (self->module.ANI_procedures, i) {
            ANI* node_i = Vector_get(&self->module.ANI_procedures, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(node, ast, walker, opt);
         }
      } return;
      
      case ANT_Procedure: {
         walker(self, opt);
         foreach (self->procedure.ANI_parameters, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_parameters, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(node, ast, walker, opt);
         }

         foreach (self->procedure.ANI_body, i) {
            ANI* node_i = Vector_get(&self->procedure.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(node, ast, walker, opt);
         }
      } return;

      case ANT_Parameter: {
         walker(self, opt);
      } return;
      
      case ANT_VariableDecl: {
         walker(self, opt);
         if (self->variable_decl.expression == -1) return;
         AstNode* node = Vector_get(&ast->AstNodes, self->variable_decl.expression);
         AstNode_visit(node, ast, walker, opt);
      } return;

      case ANT_IfStmt: {
         walker(self, opt);
         if (self->if_stmt.expression >= 0) {
            AstNode* node = Vector_get(&ast->AstNodes, self->if_stmt.expression);
            AstNode_visit(node, ast, walker, opt);
         }
         
         foreach (self->if_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->if_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(node, ast, walker, opt);
         }

         if (self->if_stmt.next_branch >= 0) {
            AstNode* node = Vector_get(&ast->AstNodes, self->if_stmt.next_branch);
            AstNode_visit(node, ast, walker, opt);
         }
      } return;

      case ANT_WhileStmt: {
         walker(self, opt);
         
         mcu_assert(self->while_stmt.expression != -1,
            "While statements must always have an expression");
         AstNode* node = Vector_get(&ast->AstNodes, self->while_stmt.expression);
         AstNode_visit(node, ast, walker, opt);
         
         foreach (self->while_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->while_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(node, ast, walker, opt);
         }
      } return;

      case ANT_BreakStmt: [[fallthrough]];
      case ANT_ContinueStmt: {
         walker(self, opt);
      } return;

      case ANT_ReturnStmt: {
         walker(self, opt);
         if (self->return_stmt.expression == -1) return;
         AstNode* node = Vector_get(&ast->AstNodes, self->return_stmt.expression);
         AstNode_visit(node, ast, walker, opt);
      } return;

      case ANT_BinOp: {
         walker(self, opt);
         AstNode* node;

         node = Vector_get(&ast->AstNodes, self->bin_op.left);
         AstNode_visit(node, ast, walker, opt);
         node = Vector_get(&ast->AstNodes, self->bin_op.right);
         AstNode_visit(node, ast, walker, opt);
      } return;
      
      case ANT_IntLiteral: [[fallthrough]];
      case ANT_StringLiteral: [[fallthrough]];
      case ANT_Variable: {
         walker(self, opt);
      } return;

      case ANT_FunctionCall: {
         walker(self, opt);
         foreach (self->function_call.ANI_arguments, i) {
            ANI* node_i = Vector_get(&self->function_call.ANI_arguments, i);
            AstNode* argument = Vector_get(&ast->AstNodes, *node_i);
            AstNode_visit(argument, ast, walker, opt);
         }
      } return;
   }

   panic("unreachable");
}

void Ast_walk(Ast* self, AstWalker walker, nullable void* opt) {
   mcu_assert(self != nullptr, "self can't be null");
   mcu_assert(walker != nullptr, "walker can't be null");

   foreach (self->AstNode_modules, i) {
      AstNode* module = Vector_get(&self->AstNode_modules, i);
      mcu_assert(module->type == ANT_Module,
         "Only module nodes should be present in AstNode_modules");
      AstNode_visit(module, self, walker, opt);
   }
}

