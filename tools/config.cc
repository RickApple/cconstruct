#include "../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration = cc.configuration.create("Debug");
  cc.workspace.addConfiguration(configuration);

  cc_project_t p = cc.project.create("cconstruct_release", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"cconstruct_release.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
#if defined(_WIN32)
  cc.state.addPreprocessorDefine(flags, "_CRT_SECURE_NO_WARNINGS");
#endif
  cc.state.disableWarningsAsErrors(flags);
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generateInFolder("build");

  return 0;
}