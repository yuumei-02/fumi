#pragma once

#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"

typedef enum {
   O_Add, O_Sub, // + -
   O_Mul, O_Div, // * /

   O_Less,    O_Great,    // < >
   O_LessEqu, O_GreatEqu, // <= >=
   O_Is,      O_IsNot,    // == !=
   O_And,     O_Or,       // && ||

   O_Equ,               // =
   O_PlusEqu, O_MinEqu, // += -=
   O_MulEqu,  O_DivEqu, // *= /=
} Operator;

typedef enum {
   OA_Left,
   OA_Right
} OperatorAssociation;

typedef enum {
   ANT_Module,
   ANT_Procedure,
   ANT_Parameter,

   ANT_VariableDecl,
   ANT_ReturnStmt,
   ANT_IfStmt,
   ANT_WhileStmt,
   ANT_BreakStmt,
   ANT_ContinueStmt,

   ANT_BinOp,
   ANT_IntLiteral,
   ANT_StringLiteral,
   ANT_Variable,
   ANT_FunctionCall
} AstNodeType;

/// AstNodeIndex
typedef isize ANI;

// @note: Don't forget to update Ast_free when changing AstNode fields
typedef struct {
   AstNodeType type;

   union {
      struct {
         String path;
         Vector ANI_procedures;
      } module;

      struct {
         String name;
         String return_type;
         Vector ANI_parameters;
         Vector ANI_body;
      } procedure;

      struct {
         String name;
         String type;
      } parameter;

      struct {
         String name;
         String type;
         ANI expression;
      } variable_decl;

      struct {
         ANI expression;
         ANI next_branch;
         Vector ANI_body;
      } if_stmt;

      struct {
         ANI expression;
         Vector ANI_body;
      } while_stmt;

      struct {
         ANI expression;
      } return_stmt;

      struct {
         Operator operator;
         ANI left;
         ANI right;
      } bin_op;

      struct {
         String function;
         Vector ANI_arguments;
      } function_call;

      i64 int_literal;
      String str_literal;
      String variable;
   };
} AstNode;

typedef struct {
   Vector AstNode_modules;
   Vector AstNodes;
} Ast;

/// [failure] is allowed to be null.
Ast Ast_parse_from_file_path(const cstr path, bool* failure);
void Ast_free(Ast* self);

void Ast_print(Ast self);

