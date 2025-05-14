#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_project_t p = cc.project.create("clean_env", CCProjectTypeConsoleApplication, NULL);
  // Files are added relative to the main CConstruct config file.
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  // Folder to generate projects in is also relative to main CConstruct config file.
  cc.generateInFolder("build");

  return 0;
}