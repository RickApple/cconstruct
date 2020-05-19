#include "../../source/cconstruct.h"

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

  cc_project_t p = cc.createProject("mixing_c_and_cpp", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.cpp", "src/function.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_default_generator("build");

  return 0;
}