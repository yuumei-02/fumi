#define VMAKE_IMPL
#include "vmake.h"
#include <mcu/handlers.h>

Vmake vmake;

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_new();
   Vmake_go_rebuild_yourself(argc, argv);

   BuildOptions build_options = {
      .compiler = GCC,
      .optimization = Og,
      .standard = C23,
      .warnings = {
         .wall = true,
         .wextra = true,
         .pedantic = true
      }
   };

   ModuleId fumi = Module_new("fumi", "./fumi", MT_Executable);
   Module_add_external_dependency(fumi, "mcu-debug");
   
   return Vmake_build(fumi, build_options);
}

