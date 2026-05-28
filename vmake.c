#define VMAKE_IMPL
#include <vmake.h>

typedef enum {
   C_None,
   C_Build,
   C_Install,
   C_Version,
   C_Help
} Command;

i32 build(BuildOptions build_options) {
   ModuleId fumi = Module_new("fumi", "./fumi", MT_Executable);
   return Vmake_build(fumi, build_options);
}

i32 install(BuildOptions build_options, const cstr install_path) {
   i32 result = build(build_options);
   if (result) return result;

   return execute_command("sudo cp ./build/bin/fumi %s/fumi", install_path);
}

i32 version() {
   printf("version: ");
   return execute_command_impl(false, false, "git rev-parse --short HEAD");
}

i32 help(Command command) {
   println("The vmake build script for the fumi compiler.\n");
   switch (command) {
      case C_Help: {
         printf("Usage:\n"
                "   vmake <command> <?flags>\n"
                "\n"
                "Commands:\n"
                "   build     Builds the compiler, putting the result in ./build/bin/ under the name fumi.\n"
                "   install   Builds and installs the compiler. The default path is /usr/local/bin/\n"
                "   version   Print out the current version (git hash of HEAD).\n"
                "   help      This help message.\n"
                "\n"
                "See \"vmake help <?command>\" for more information on a specific command.\n");
      } return 0;

      case C_Build: {
         printf("Usage:\n"
                "   vmake build <?profile>\n"
                "\n"
                "Profiles:\n"
                "   --release\n"
                "   --debug\n"
                "\n"
                "Defaults:\n"
                "   <profile>   --debug\n"
                "\n"
                "See \"vmake help <?command>\" for more information on a specific command.\n");
      } return 0;

      case C_Install: {
         printf("Usage:\n"
                "   vmake install <?profile> <?flags>\n"
                "\n"
                "Profiles:\n"
                "   --release\n"
                "   --debug\n"
                "\n"
                "Flags:\n"
                "   -p <path>   Set the install path.\n"
                "\n"
                "Defaults:\n"
                "   <profile>   --debug\n"
                "   -p          /usr/local/bin/\n"
                "\n"
                "See \"vmake help <?command>\" for more information on a specific command.\n");
      } return 0;

      case C_Version: {
         println("No further information on the `version` command");
      } return 0;

      case C_None: return 0;
   }
   panic("unreachable");
}

i32 main(i32 argc, cstr argv[]) {
   Vmake_go_rebuild_yourself(argc, argv);

   i32 install_path = -1;
   bool help_set = false;
   Command command = C_None;
   BuildOptions build_options = BuildOptions_default_debug();

   for (i32 i = 1; i < argc - 1; ++i) {
      cstr_match(argv[i]) {
         ncstreq("build")  command = C_Build;
         cstreq("install") command = C_Install;
         cstreq("version") command = C_Version;
         cstreq("help") {
            help_set = true;
            command = C_Help;
         }
         
         cstreq("-i") {
            if (!(i + 1 < argc)) {
               eprintln("[!] No install path provided for the -i flag");
               return 1;
            }
            install_path = i + 1;
         }
         
         cstreq("--debug")   build_options = BuildOptions_default_debug();
         cstreq("--release") build_options = BuildOptions_default_release();
         
         else {
            i32 result = help(C_Help);
            println("[!] Unknown command \"%s\"", argv[i]);
            return result;
         }
      }
   }

   switch (command) {
      case C_Build: {
         if (help_set)
            return help(C_Build);
         return build(build_options);
      }
      
      case C_Install: {
         if (help_set)
            return help(C_Install);
         return install(build_options, install_path > 0 ? argv[install_path] : "/usr/local/bin");
      }

      case C_Version: {
         if (help_set)
            return help(C_Version);
         return version();
      }

      case C_None: [[fallthrough]];
      case C_Help: {
         return help(C_Help);
      }
   }

   panic("unreachable");
}

