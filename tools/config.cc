#include "../source/cconstruct.h"

int main() {
  CCPlatformHandle platform           = cc.createPlatform(EPlatformTypeX64);
  CCConfigurationHandle configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  void* p = cc.createProject("cconstruct_release", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"cconstruct_release.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_flags flags = {0};
  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "_CRT_SECURE_NO_WARNINGS");
#if defined(__APPLE__)
  array_push(flags.compile_options, "-std=c++11");
#endif
  cc.project.setFlags(p, &flags);

  cc_default_generator("build");

  return 0;
}