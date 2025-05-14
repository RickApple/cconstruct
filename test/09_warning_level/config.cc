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

  cc_project_t p      = cc.project.create("warning_level", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  // Debug build gives warnings which are by default turned into errors.
  // Disabling warnings as errors still shows the warnings during compile, but no errors.
  cc.state.disableWarningsAsErrors(flags);
  cc.project.setFlags(p, flags, arch, configuration_debug);

  cc.state.reset(flags);
  // Another way to prevent these warnings is to turn down the warning level, in this case all the
  // way off. NOT RECOMMENDED!
  cc.state.setWarningLevel(flags, EStateWarningLevelNone);
  cc.project.setFlags(p, flags, arch, configuration_release);

  // TODO: show case the different warning levels with different settings for different files
  cc.generateInFolder("build");

  return 0;
}