#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("test");

  auto platform_x64 = cc.platform.create(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  auto configuration_debug   = cc.configuration.create("Debug");
  auto configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  auto p              = cc.project.create("include_folders", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c", "src/function.c"};
  cc.project.addFiles(p, "Source Files", countof(files), files);

  cc_flags flags;
  cc.state.reset(&flags);
  array_push(flags.include_folders, "../src/include");  // TODO: fix the relativeness of this folder
  cc.project.setFlags(p, &flags);

  cc_default_generator("build");

  return 0;
}