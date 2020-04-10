#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("${platform}/${configuration}");

  CCPlatformHandle platform                   = cc.createPlatform(EPlatformTypeX64);
  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_flags flags      = {0};
  void* p             = cc.createProject("preprocessor", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, "Source Files", countof(files), files);

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=5");
  cc.project.setFlagsLimited(p, &flags, platform, configuration_debug);
  // Modifying flags after it has been set on a project does not affect the project anymore
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=6");

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=4");
  cc.project.setFlagsLimited(p, &flags, platform, configuration_release);

  cc_default_generator("build");

  return 0;
}