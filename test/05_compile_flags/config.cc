#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
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

  cc_state_t flags = cc.createState();
  // Beware, there are more general ways to set warning flags, see other test case.
#ifdef WIN32
  cc.state.addCompilerFlag(flags, "/WX");
#else
  cc.state.addCompilerFlag(flags, "-Werror");
  cc.state.addCompilerFlag(flags, "-Wunused-variable");
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc_default_generator("build");

  return 0;
}