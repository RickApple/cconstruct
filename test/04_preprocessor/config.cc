#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");

  cc_platform_t platform                   = cc.createPlatform(EPlatformTypeX64);
  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_flags flags      = {0};
  cc_project_t p      = cc.createProject("preprocessor", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=5");
  cc.project.setFlags(p, &flags, platform, configuration_debug);
  // Modifying flags after it has been set on a project does not affect the project anymore
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=6");

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=4");
  cc.project.setFlags(p, &flags, platform, configuration_release);

  cc_default_generator("build");

  return 0;
}