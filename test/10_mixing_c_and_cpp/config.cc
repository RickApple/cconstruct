#include "../../source/cconstruct.h"

int main() {
  CCPlatformHandle platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  void* p = cc.createProject("mixing_c_and_cpp", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.cpp", "src/function.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_default_generator("build");

  return 0;
}