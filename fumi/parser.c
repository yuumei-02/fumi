#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"
#include "lexer.h"
#include "parser.h"
#include "reporter.h"

typedef struct {
   bool panic_mode;
   bool failure;
} ParseState;

#define enter_panic() \
   state->panic_mode = true; \
   state->failure = true;

#define exit_panic() \
   state->panic_mode = false;

/// [is_operator] is allowed to be null
Operator TokenType_to_operator(TokenType type, bool* is_operator) {
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

ANI parse_expression_impl(Lexer* lexer, Ast* ast, ParseState* state, isize precendence);

ANI parse_expression(Lexer* lexer, Ast* ast, ParseState* state) {
   Token peek = Lexer_next(lexer);
   if (peek.type == TT_NewLine) return -1;
   Lexer_undo(lexer, peek);
   
   return parse_expression_impl(lexer, ast, state, -1);
}

ANI parse_atom(Lexer* lexer, Ast* ast, ParseState* state) {
retry:
   Token token = Lexer_next(lexer);

   switch (token.type) {
      case TT_IntLiteral: {
         Vector_push_create(&ast->AstNodes, ((AstNode) {
            .type = ANT_IntLiteral,
            .int_literal = token.int_literal
         }));

         return (ANI) (ast->AstNodes.length - 1);
      }

      case TT_StringLiteral: {
         Vector_push_create(&ast->AstNodes, ((AstNode) {
            .type = ANT_StringLiteral,
            .str_literal = token.str_literal
         }));

         return (ANI) (ast->AstNodes.length - 1);      
      }

      case TT_Identifier: {
         Token next = Lexer_next(lexer);

         switch (next.type) {
            case TT_LParen: {
               AstNode function_call = {
                  .type = ANT_FunctionCall,
                  .function_call = {
                     .function = token.str_literal,
                     .ANI_arguments = Vector_new(sizeof(ANI))
                  }
               };

               // @todo: validate
               bool expecting_comma = false;
               loop {
                  next = Lexer_next(lexer);
                  switch (next.type) {
                     case TT_RParen: {
                        exit_panic();
                        Vector_push(&ast->AstNodes, &function_call);
                        return (ANI) (ast->AstNodes.length - 1);
                     } break;

                     case TT_Comma: {
                        if (expecting_comma) {
                           exit_panic();
                           expecting_comma = false;
                           break;
                        }

                        if (!state->panic_mode) {
                           enter_panic();
                           report_unexpected_token(lexer->path, next);
                        }
                     } break;

                     default: {
                        if (expecting_comma) goto skip_expression;
                     
                        Lexer_undo(lexer, next);
                        ANI arg = parse_expression(lexer, ast, state);
                        if (arg >= 0) {
                           exit_panic();
                           expecting_comma = true;
                           Vector_push(&function_call.function_call.ANI_arguments, &arg);
                           break;
                        }
                        next = Lexer_next(lexer);

                     skip_expression:
                        Token_free(next);
                        if (!state->panic_mode) {
                           report_unexpected_token(lexer->path, next);
                           enter_panic();
                        }

                        if (next.type == TT_Eof) {
                           return -1;
                        }
                     }
                  }
               }
            } break;
            
            default: {
               Vector_push_create(&ast->AstNodes, ((AstNode) {
                  .type = ANT_Variable,
                  .variable = token.str_literal
               }));

               Lexer_undo(lexer, next);
               return (ANI) (ast->AstNodes.length - 1);
            }
         }
      }

      case TT_NewLine: goto retry;

      default: {
         if (!state->panic_mode) {
            report_unexpected_token(lexer->path, token);
            enter_panic();
         }
         Token_free(token);
         return -1;
      }
   }
}

// @reference: https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
ANI parse_expression_impl(Lexer* lexer, Ast* ast, ParseState* state, isize precendence) {
   ANI lhs = parse_atom(lexer, ast, state);
   if (lhs == -1) return lhs;
   
   loop {
      Token op_token = Lexer_next(lexer);

      bool is_op;
      Operator op = TokenType_to_operator(op_token.type, &is_op);
      if (!is_op || Operator_get_precedence(op) < precendence) {
         Lexer_undo(lexer, op_token);
         break;
      }

      isize next_precedence = Operator_get_precedence(op);
      if (Operator_get_association(op) == OA_Left)
         next_precedence += 1;

      ANI rhs = parse_expression_impl(lexer, ast, state, next_precedence);

      switch (op) {
         case O_Add:      [[fallthrough]];
         case O_Sub:      [[fallthrough]];
         case O_Mul:      [[fallthrough]];
         case O_Div:      [[fallthrough]];
         case O_Less:     [[fallthrough]];
         case O_Great:    [[fallthrough]];
         case O_LessEqu:  [[fallthrough]];
         case O_GreatEqu: [[fallthrough]];
         case O_Is:       [[fallthrough]];
         case O_IsNot:    [[fallthrough]];
         case O_And:      [[fallthrough]];
         case O_Or:       [[fallthrough]];
         case O_Equ:      [[fallthrough]];
         case O_PlusEqu:  [[fallthrough]];
         case O_MinEqu:   [[fallthrough]];
         case O_MulEqu:   [[fallthrough]];
         case O_DivEqu: {
            Vector_push_create(&ast->AstNodes, ((AstNode) {
               .type = ANT_BinOp,
               .bin_op = {
                  .operator = op,
                  .left = lhs,
                  .right = rhs
               }
            }));
            lhs = (ANI) (ast->AstNodes.length - 1);
         } break;
      }
   }

   return lhs;
}

ANI parse_variable_decl(String name, Lexer* lexer, Ast* ast, ParseState* state) {
   AstNode self = {
      .type = ANT_VariableDecl,
      .variable_decl = {
         .name = name,
         .expression = -1
      }
   };

   Token token = Lexer_next(lexer);

   if (token.type == TT_Identifier) {
      self.variable_decl.type = token.str_literal;
      token = Lexer_next(lexer);

      if (token.type != TT_Equals) {
         Lexer_undo(lexer, token);
         goto success;
      }
   } else if (token.type == TT_Equals) {
      self.variable_decl.type = String_from("@infer");
   } else {
      report_unexpected_token(lexer->path, token);
      enter_panic();
      goto failure;
   }

   self.variable_decl.expression = parse_expression(lexer, ast, state);

success:
   Vector_push(&ast->AstNodes, &self);
   return (ANI) (ast->AstNodes.length - 1);

failure:
   String_free(&name);
   return -1;
}

ANI parse_return_stmt(Lexer* lexer, Ast* ast, ParseState* state) {
   AstNode self = {
      .type = ANT_ReturnStmt,
      .return_stmt = {
         .expression = parse_expression(lexer, ast, state)
      }
   };

   Vector_push(&ast->AstNodes, &self);
   return (ANI) (ast->AstNodes.length - 1);
}

/// Returns a [Vector<ANI>]
Vector parse_code_block(Lexer* lexer, Ast* ast, ParseState* state);

ANI parse_if_stmt(Lexer* lexer, Ast* ast, ParseState* state) {
   AstNode self = {
      .type = ANT_IfStmt
   };

   self.if_stmt.expression = parse_expression(lexer, ast, state);
   if (self.if_stmt.expression < 0) {
      enter_panic();
      return -1;
   }

   Token next = Lexer_next(lexer);
   if (next.type != TT_Then) {
      enter_panic();
      report_unexpected_token_expected(lexer->path, next, TT_Then);
      return -1;
   }

   self.if_stmt.ANI_body = parse_code_block(lexer, ast, state);
   
   Vector_push(&ast->AstNodes, &self);
   return (ANI) (ast->AstNodes.length - 1);
}

ANI parse_while_stmt(Lexer* lexer, Ast* ast, ParseState* state) {
   AstNode self = {
      .type = ANT_WhileStmt
   };

   self.while_stmt.expression = parse_expression(lexer, ast, state);
   if (self.while_stmt.expression < 0) {
      enter_panic();
      return -1;
   }

   Token next = Lexer_next(lexer);
   if (next.type != TT_Do) {
      enter_panic();
      report_unexpected_token_expected(lexer->path, next, TT_Do);
      return -1;
   }

   self.while_stmt.ANI_body = parse_code_block(lexer, ast, state);
   
   Vector_push(&ast->AstNodes, &self);
   return (ANI) (ast->AstNodes.length - 1);
}

/// Returns a [Vector<ANI>]
Vector parse_code_block(Lexer* lexer, Ast* ast, ParseState* state) {
   Vector self = Vector_new(sizeof(ANI));

   loop {
      Token token = Lexer_next(lexer);

      switch (token.type) {
         case TT_Identifier: {
            Token peek = Lexer_peek(lexer);

            switch (peek.type) {
               case TT_Colon: {
                  exit_panic();
                  Lexer_next(lexer);
                  ANI variable_decl = parse_variable_decl(token.str_literal, lexer, ast, state);
                  if (variable_decl >= 0) Vector_push(&self, &variable_decl);
               } break;

               // @todo: validate correctness
               default: {
                  Lexer_undo(lexer, token);
                  ANI expression = parse_expression(lexer, ast, state);
                  if (expression >= 0) {
                     Vector_push(&self, &expression);
                     break;
                  }

                  Lexer_next(lexer);
                  if (!state->panic_mode) {
                     report_unexpected_token(lexer->path, peek);
                     enter_panic();
                  }
                  
                  Token_free(token);
                  break;
               }
            }
         } break;

         case TT_Return: {
            exit_panic();
            ANI return_stmt = parse_return_stmt(lexer, ast, state);
            Vector_push(&self, &return_stmt);
         } break;

         case TT_If: {
            exit_panic();
            ANI if_stmt = parse_if_stmt(lexer, ast, state);
            if (if_stmt >= 0) Vector_push(&self, &if_stmt);
         } break;

         case TT_While: {
            exit_panic();
            ANI while_stmt = parse_while_stmt(lexer, ast, state);
            if (while_stmt >= 0) Vector_push(&self, &while_stmt);
         } break;

         case TT_Break: {
            AstNode break_stmt = { .type = ANT_BreakStmt };
            Vector_push(&ast->AstNodes, &break_stmt);
            ANI ani_break_stmt = (ANI) (ast->AstNodes.length - 1);
            Vector_push(&self, &ani_break_stmt);
         } break;

         case TT_Continue: {
            AstNode continue_stmt = { .type = ANT_ContinueStmt };
            Vector_push(&ast->AstNodes, &continue_stmt);
            ANI ani_continue_stmt = (ANI) (ast->AstNodes.length - 1);
            Vector_push(&self, &ani_continue_stmt);
         } break;
      
         case TT_End: {
            exit_panic();
            return self;
         }

         case TT_Eof: {
            enter_panic();
            report_unexpected_token(lexer->path, token);
            return self;
         }

         case TT_NewLine: break;

         default: {
            Lexer_undo(lexer, token);
            ANI expression = parse_expression(lexer, ast, state);
            if (expression >= 0) {
               Vector_push(&self, &expression);
               break;
            }

            Lexer_next(lexer);
            Token_free(token);
            if (state->panic_mode) break;
            enter_panic();
            report_unexpected_token(lexer->path, token);
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
      Token token = Lexer_next(lexer);
      if (token.type == TT_NewLine) continue;
      
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
   Lexer lexer = Lexer_new(path);

   AstNode self = {
      .type = ANT_Module,
      .module = {
         .path = String_from((cstr) path),
         .ANI_procedures = Vector_new(sizeof(ANI))
      }
   };
   
   loop {
      Token token = Lexer_next(&lexer);

      switch (token.type) {
         case TT_Procedure: {
            exit_panic();
            ANI procedure = parse_procedure(&lexer, ast, state);
            if (procedure == -1) break;
            Vector_push(&self.module.ANI_procedures, &procedure);
         } break;

         case TT_NewLine: break;
      
         case TT_Eof: {
            exit_panic();
            goto finish_parsing;
         }
         
         default: {
            Token_free(token);
            if (state->panic_mode) break;
            enter_panic();
            report_unexpected_token(lexer.path, token);
         }
      }
   }

finish_parsing:
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
   if (state.failure) goto failure;

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
         indprintln("├─expression:");
         
         mcu_assert(self->if_stmt.expression != -1, "If statements must always have an expression");
         AstNode* node = Vector_get(&ast->AstNodes, self->if_stmt.expression);
         AstNode_print(node, ast, indent + 1);
         
         if (self->if_stmt.ANI_body.length <= 0) {
            indprintln("└─body: empty");
         } else {
            indprintln("└─body:");
         }

         foreach (self->if_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->if_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
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
         }

         foreach (self->while_stmt.ANI_body, i) {
            ANI* node_i = Vector_get(&self->while_stmt.ANI_body, i);
            AstNode* node = Vector_get(&ast->AstNodes, *node_i);
            AstNode_print(node, ast, indent + 1);
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

void Ast_print(Ast self) {
   foreach (self.AstNode_modules, i) {
      AstNode* node = Vector_get(&self.AstNode_modules, i);
      AstNode_print(node, &self, 0);
   }
}

