#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug   = cc.configuration.create("Debug");
  cc_configuration_t configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p      = cc.project.create("compile_flags", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  // Beware, there are more general ways to set warning flags, see other test case.
#ifdef WIN32
  cc.state.addCompilerFlag(flags, "/WX");
  cc.state.addCompilerFlag(flags, "/Wall");
#else
  cc.state.addCompilerFlag(flags, "-Werror");
  cc.state.addCompilerFlag(flags, "-Wunused-variable");
  if (cc.generateInFolder == cc.generator.xcode) {
    cc.state.addCompilerFlag(flags, "MACOSX_DEPLOYMENT_TARGET=10.15");
  }
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generateInFolder("build");

  return 0;
}