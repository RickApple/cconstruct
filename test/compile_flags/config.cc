#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("test");

  auto platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  auto configuration_debug   = cc.createConfiguration("Debug");
  auto configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  auto p              = cc.createProject("compile_flags", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, "Source Files", countof(files), files);

  cc_flags flags;
  cc.state.reset(&flags);
#ifdef WIN32
  array_push(flags.compile_options, "/WX");
#else
  array_push(flags.compile_options, "-Werror");
  array_push(flags.compile_options, "-Wunused-variable");
#endif
  cc.project.setFlags(p, &flags);

  cc_default_generator("build");

  return 0;
}