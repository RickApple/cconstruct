#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.createArchitecture(EArchitectureX64);
  cc_platform_t platform = cc.createPlatform(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug = cc.createConfiguration("Debug");
  cc.workspace.addConfiguration(configuration_debug);

  cc_project_t p =
      cc.createProject("cconstruct_architecture", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.createState();
#ifdef _M_IX86
  cc.state.addCompilerFlag(flags, "/DOUTPUT=86");
  cc.project.setOutputFolder(p, "x86");
#elif _M_X64
  cc.state.addCompilerFlag(flags, "/DOUTPUT=64");
  cc.project.setOutputFolder(p, "amd64");
#elif _M_IA64
  cc.state.addCompilerFlag(flags, "/DOUTPUT=65");
  cc.project.setOutputFolder(p, "ia64");
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc_default_generator("build");

  return 0;
}