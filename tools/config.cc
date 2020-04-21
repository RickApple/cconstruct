#include "../source/cconstruct.h"

int main(int argc, char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_platform_t platform           = cc.createPlatform(EPlatformTypeX64);
  cc_configuration_t configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  cc_project_t p = cc.createProject("cconstruct_release", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"cconstruct_release.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_flags flags = {0};
  cc.state.reset(&flags);
#if defined(_WIN32)
  cc.state.addPreprocessorDefine(&flags, "_CRT_SECURE_NO_WARNINGS");
#endif
  cc.project.setFlags(p, &flags, NULL, NULL);

  cc_default_generator("build");

  return 0;
}