#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX86);
  cc.workspace.addArchitecture(arch);
  arch = cc.architecture.create(EArchitectureX64);
  cc.workspace.addArchitecture(arch);

  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug = cc.configuration.create("Debug");
  cc.workspace.addConfiguration(configuration_debug);

  cc_project_t p =
      cc.project.create("cconstruct_architecture", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
#ifdef _M_IX86
  cc.state.addCompilerFlag(flags, "/DOUTPUT=86");
  cc.project.setOutputFolder(p, "x86");
#elif _M_X64
  cc.state.addCompilerFlag(flags, "/DOUTPUT=64");
  cc.project.setOutputFolder(p, "x64");
#elif _M_IA64
  cc.state.addCompilerFlag(flags, "/DOUTPUT=65");
  cc.project.setOutputFolder(p, "ia64");
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generator.standard("build");

  return 0;
}