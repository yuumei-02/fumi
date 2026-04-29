#include <mcu/core.h>
#include <mcu/containers.h>

typedef enum {
   ANT_Module,
   ANT_Procedure,
   
   ANT_VariableDecl,

   ANT_IntLiteral
} AstNodeType;

/// AstNodeIndex
typedef isize ANI;

typedef struct {
   AstNodeType type;

   union {
      struct {
         String path;
         Vector ANI_functions;
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

void Ast_print(Ast* self);

