#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("test");

  auto platform              = cc.platform.create("Win32", EPlatformTypeX86);
  auto configuration_debug   = cc.configuration.create("Debug");
  auto configuration_release = cc.configuration.create("Release");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_flags flags = {};

  auto p              = cc.project.create("preprocessor", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c", nullptr};
  cc.project.addFiles(p, "Source Files", files);

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=5");
  cc.project.setFlagsLimited(p, &flags, platform, configuration_debug);

  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=4");
  cc.project.setFlagsLimited(p, &flags, platform, configuration_release);

  cc_default_generator("build");

  return 0;
}