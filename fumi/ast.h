#pragma once

#include <mcu/core.h>
#include <mcu/containers.h>

#include "lexer.h"

/// AstNodeIndex
typedef isize ANI;

typedef enum {
   Bit64,
   Bit32,
   Bit16,
   Bit8
} BitLength;

typedef enum {
   TK_Void,
   TK_Int,
} TypeKind;

typedef struct {
   TypeKind kind;
   union {
      struct {
         BitLength bits;
         bool is_signed;
      } integer;
   };
} Type;

typedef struct {
   ANI node;
} Procedure;

typedef struct {
   ANI node;
} Variable;

typedef enum {
   SK_Proc,
   SK_Type,
   SK_Var
} SymbolKind;

typedef struct {
   SymbolKind kind;
   union {
      Type type;
      Procedure procedure;
      Variable var;
   };
} Symbol;

HashMap_hdr(Symbol)
typedef HashMap(Symbol) SymbolTable;

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

typedef enum {
   ANS_Unchecked,
   ANS_Valid,
   ANS_Poison
} AstNodeStatus;

// @note: Don't forget to update Ast_free when changing AstNode fields
typedef struct {
   AstNodeType type;
   AstNodeStatus status;
   cstr path;
   usize x;
   usize y;

   union {
      struct {
         String path;
         String name;
         Vector ANI_procedures;
         SymbolTable scope;
      } module;

      struct {
         Token name;
         Token return_type;
         Vector ANI_parameters;
         Vector ANI_body;
         SymbolTable scope;
      } procedure;

      struct {
         Token name;
         Token type;
      } parameter;

      struct {
         Token name;
         Token type;
         ANI expression;
      } variable_decl;

      struct {
         ANI expression;
         ANI next_branch;
         Vector ANI_body;
         SymbolTable scope;
      } if_stmt;

      struct {
         ANI expression;
         Vector ANI_body;
         SymbolTable scope;
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
   SymbolTable global_scope;
   Vector AstNode_modules;
   Vector AstNodes;
} Ast;

const cstr BitLength_to_cstr(BitLength self);
const cstr TypeKind_to_cstr(TypeKind self);
const cstr SymbolKind_to_cstr(SymbolKind self);
const cstr AstNodeStatus_to_cstr(AstNodeStatus self);

Operator TokenType_to_operator(TokenType type, nullable bool* is_operator);
const cstr Operator_to_cstr(Operator operator);
isize Operator_get_precedence(Operator operator);
OperatorAssociation Operator_get_association(Operator self);

void Ast_free(Ast* self);
void Ast_print(Ast self);

typedef void (*AstWalker)(AstNode* node, bool exiting, nullable void* opt);
void Ast_walk(Ast* self, AstWalker walker, nullable void* opt);

