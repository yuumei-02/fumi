#pragma once

#include <mcu/core.h>
#include <mcu/containers.h>

#include "flags.h"

typedef enum {
   O_Add,
   O_Sub,
   O_Div,
   O_Mul,
} Operator;

typedef enum {
   OA_Left,
   OA_Right
} OperatorAssociation;

typedef enum {
   ANT_Module,
   ANT_Procedure,

   ANT_VariableDecl,

   ANT_BinOp,
   ANT_IntLiteral,
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
         Vector ANI_body;
      } procedure;

      struct {
         String name;
         String type;
         ANI expression;
      } variable_decl;

      struct {
         Operator operator;
         ANI left;
         ANI right;
      } bin_op;

      i64 int_literal;
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

