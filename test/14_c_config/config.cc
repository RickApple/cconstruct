#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  struct C_struct s = {.debug_name = "Debug", .release_name = "Release"};

  cc_configuration_t configuration_debug   = cc.configuration.create(s.debug_name);
  cc_configuration_t configuration_release = cc.configuration.create(s.release_name);
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p = cc.project.create("c_config", CCProjectTypeConsoleApplication, NULL);

  const char* files[] = {"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.generateInFolder("build");

  return 0;
}