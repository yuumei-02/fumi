#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "typing.h"

HashMap_impl(Symbol)

void Ast_create_symbol_tables(Ast* self) {
   mcu_assert(self != nullptr, "self can't be null");
}

