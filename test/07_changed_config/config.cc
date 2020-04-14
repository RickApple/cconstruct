#include "../../source/cconstruct.h"

int main(int argc, const char* argv[]) {
  // This line needs to be added to your config file to allow CConstruct to pick up changes in your
  // config file. It will simply always create a new version of itself, before running that new
  // version.
  cc.autoRecompileFromConfig(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");

  CCPlatformHandle platform           = cc.createPlatform(EPlatformTypeX64);
  CCConfigurationHandle configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  cc_flags flags      = {0};
  void* p             = cc.createProject("changed_config", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.state.reset(&flags);
#include "return_value.inl"
  cc.project.setFlagsLimited(p, &flags, platform, configuration);

  cc_default_generator("build");

  return 0;
}