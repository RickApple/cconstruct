#include <vector>

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

  cc_project_t p = cc.project.create("cpp_config", CCProjectTypeConsoleApplication, NULL);
  // Files are added relative to the main CConstruct config file.
  std::vector<const char*> files{"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, (unsigned int)files.size(), files.data(), NULL);

  cc.generateInFolder("build");

  return 0;
}