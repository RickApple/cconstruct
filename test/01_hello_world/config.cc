#include "../../source/cconstruct.h"

int main(int argc, char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_platform_t platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p = cc.createProject("hello_world", CCProjectTypeConsoleApplication, NULL);
  // Files are added relative to the main CConstruct config file.
  const char* files[] = {"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  // Folder to generate projects in is also relative to main CConstruct config file.
  cc_default_generator("build");

  return 0;
}