#include "flags.h"

CompileFlags CompileFlags_default() {
   return (CompileFlags) {
      .token_dump = false,
      .ast_dump   = false,
      .doc_gen    = false
   };
}

