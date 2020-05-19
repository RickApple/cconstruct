#include "../../../source/cconstruct.h"

// Subfolder includes
#include "function/config.cc"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.createArchitecture(EArchitectureX64);
  cc_platform_t platform = cc.createPlatform(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p      = cc.createProject("nested_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.createState();
  cc.state.addIncludeFolder(flags, "function");
  cc.project.setFlags(p, flags, NULL, NULL);

  add_function(cc, p);

  // Note that the folder to generate the project is is relative to the main CConstruct config
  // file.
  cc_default_generator("../build");

  return 0;
}