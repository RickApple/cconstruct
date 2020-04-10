#include "../../source/cconstruct.h"

int main() {
  CCPlatformHandle platform_x86 = cc.createPlatform(EPlatformTypeX86);
  CCPlatformHandle platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x86);
  cc.workspace.addPlatform(platform_x64);

  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  void* p             = cc.createProject("hello_world", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, "Source Files", countof(files), files);

  cc_default_generator("build");

  return 0;
}