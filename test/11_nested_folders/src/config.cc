#include "../../../source/cconstruct.h"

// Subfolder includes
#include "function/config.cc"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_project_t p      = cc.project.create("nested_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  cc.state.addIncludeFolder(flags, "function");
  cc.project.setFlags(p, flags, NULL, NULL);

  add_function(cc, p);

  // Note that the folder to generate the project is relative to the main CConstruct config
  // file.
  cc.generateInFolder("../build");

  return 0;
}