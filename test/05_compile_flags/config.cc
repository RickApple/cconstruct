#include "../../source/cconstruct.h"

int main(int argc, char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_platform_t platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p      = cc.createProject("compile_flags", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_flags flags;
  cc.state.reset(&flags);
  // Beware, there are more general ways to set warning flags, see other test case.
#ifdef WIN32
  array_push(flags.compile_options, "/WX");
#else
  array_push(flags.compile_options, "-Werror");
  array_push(flags.compile_options, "-Wunused-variable");
#endif
  cc.project.setFlags(p, &flags, NULL, NULL);

  cc_default_generator("build");

  return 0;
}