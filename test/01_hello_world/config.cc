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

  cc_project_t p = cc.project.create("hello_world", CCProjectTypeConsoleApplication, NULL);
  // Files are added relative to the main CConstruct config file.
  const char* files[] = {"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  // Folder to generate projects in is also relative to main CConstruct config file.
  cc.generator.standard("build");

  return 0;
}