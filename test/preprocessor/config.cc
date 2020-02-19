#include "../../source/cconstruct.h"

int main() {
  auto cc = cc_default;

  cc.workspace.setOutputFolder("test");

  cc.workspace.addPlatform("Win32", EPlatformTypeX86);
  cc.workspace.addConfiguration("Debug");

  cc_flags flags = {};
  cc.state.reset(&flags);
  cc.state.addPreprocessorDefine(&flags, "TEST_VALUE=4");

  auto p              = cc.project.create("preprocessor", CCProjectTypeConsoleApplication);
  const char* files[] = {"src/main.c", nullptr};
  cc.project.addFiles(p, "Source Files", files);
  cc.project.setFlags(p, &flags);

  cc.workspace.addProject(p);

  cc.generateInFolder("build");

  return 0;
}