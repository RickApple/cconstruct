#include "../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("build/release");

  auto platform      = cc.createPlatform(EPlatformTypeX64);
  auto configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  auto p              = cc.createProject("cconstruct_release", CCProjectTypeConsoleApplication);
  const char* files[] = {"cconstruct_release.cpp"};
  cc.project.addFiles(p, "Source Files", countof(files), files);

  cc_flags flags = {};
  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "_CRT_SECURE_NO_WARNINGS");
  cc.project.setFlags(p, &flags);

  cc_default_generator("build");

  return 0;
}