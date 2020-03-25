#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("test");

  auto platform_x86 = cc.platform.create(EPlatformTypeX86);
  auto platform_x64 = cc.platform.create(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x86);
  cc.workspace.addPlatform(platform_x64);

  auto configuration_debug   = cc.configuration.create("Debug");
  auto configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  auto p              = cc.project.create("hello_world", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c", "src/function.c", NULL};
  cc.project.addFiles(p, "Source Files", files);

  cc_default_generator("build");

  return 0;
}