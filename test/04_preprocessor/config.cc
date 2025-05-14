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

  cc_state_t flags    = cc.state.create();
  cc_project_t p      = cc.project.create("preprocessor", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.state.addPreprocessorDefine(flags, "TEST_VALUE=5");
  cc.project.setFlags(p, flags, arch, configuration_debug);
  // Modifying flags after it has been set on a project does not affect the project anymore
  cc.state.addPreprocessorDefine(flags, "TEST_VALUE=6");

  cc.state.reset(flags);
  cc.state.addPreprocessorDefine(flags, "TEST_VALUE=4");
  cc.project.setFlags(p, flags, arch, configuration_release);

  cc.generateInFolder("build");

  return 0;
}