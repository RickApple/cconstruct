#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug   = cc.configuration.create("Debug");
  cc_configuration_t configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p      = cc.project.create("include_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c", "src/function.c", "src/include/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  // Include folders are assumed to be relative to main CConstruct config file
  cc.state.addIncludeFolder(flags, "src/include");
  cc.project.setFlags(p, flags, NULL, NULL);

  cc_default_generator("build");

  return 0;
}