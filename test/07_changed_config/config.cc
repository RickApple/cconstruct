#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");

  cc_platform_t platform           = cc.createPlatform(EPlatformTypeX64);
  cc_configuration_t configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  cc_flags flags      = {0};
  cc_project_t p      = cc.createProject("changed_config", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.state.reset(&flags);
#include "return_value.inl"
  cc.project.setFlags(p, &flags, platform, configuration);

  cc_default_generator("build");

  return 0;
}