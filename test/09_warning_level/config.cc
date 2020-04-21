#include "../../source/cconstruct.h"

int main(int argc, char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_platform_t platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p      = cc.createProject("warning_level", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_flags flags;
  cc.state.reset(&flags);
  // Debug build gives warnings which are by default turned into errors.
  // Disabling warnings as errors still shows the warnings during compile, but no errors.
  cc.state.disableWarningsAsErrors(&flags);
  cc.project.setFlags(p, &flags, platform_x64, configuration_debug);
  cc.state.reset(&flags);
  // Another way to prevent these warnings is to turn down the warning level, in this case all the
  // way off. NOT RECOMMENDED!
  cc.state.setWarningLevel(&flags, EStateWarningLevelNone);
  cc.project.setFlags(p, &flags, platform_x64, configuration_release);

  // TODO: show case the different warning levels with different settings for different files
  cc_default_generator("build");

  return 0;
}