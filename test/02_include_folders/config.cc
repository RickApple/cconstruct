#include "../../source/cconstruct.h"

int main() {
  CCPlatformHandle platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  void* p             = cc.createProject("include_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c", "src/function.c", "src/include/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_flags flags;
  cc.state.reset(&flags);
  array_push(flags.include_folders,
             "../src/include");  // TODO: fix the relativeness of this folder
  cc.project.setFlags(p, &flags);

  cc_default_generator("build");

  return 0;
}