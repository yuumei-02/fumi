#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");

   /* foreach (self->AstNode_modules, i) { */
   /*    AstNode* module = Vector_get(&self->AstNode_modules, i); */
   /*    module->module.scope = HashMap_new(Symbol)(); */

   /*    foreach (module->module.ANI_procedures, i) { */
   /*       ANI* proc_id = Vector_get(&module->module.ANI_procedures, i); */
   /*       AstNode* proc = Vector_get(&self->AstNodes, *proc_id); */

   /*       HashMap_put(Symbol)(&module->module.scope, */
   /*          proc->procedure.name, */
   /*          (Symbol) { */
   /*             .kind = SK_Proc */
   /*          }); */
   /*    } */
   /* } */
}

