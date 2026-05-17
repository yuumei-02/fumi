#define VMAKE_IMPL
#include <vmake.h>

Vmake vmake;

i32 build(BuildOptions build_options) {
   ModuleId fumi = Module_new("fumi", "./fumi", MT_Executable);

   return Vmake_build(fumi, build_options);
}

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_go_rebuild_yourself(argc, argv);

   typedef enum {
      C_None,
      C_Build
   } Command;

   Command command = C_None;
   BuildOptions build_options = BuildOptions_default_debug();

   for (i32 i = 1; i < argc - 1; ++i) {
      cstr_match(argv[i]) {
         ncstreq("build") {
            command = C_Build;
         }
         cstreq("--debug") {
            build_options = BuildOptions_default_debug();
         }
         cstreq("--release") {
            build_options = BuildOptions_default_release();
         }
         else {
            eprintln("[!] Unknown command \"%s\"", argv[i]);
            return 1;
         }
      }
   }

   switch (command) {
      case C_Build: {
         return build(build_options);
      }
      case C_None: {
         return 0;
      }
   }

   panic("unreachable");
}

