#include "../../source/cconstruct.h"

int main() {
#if defined(_MSC_VER)
  auto cc = cc_vs2019_builder;
#else
  auto cc = cc_xcode_builder;
#endif
  cc.workspace.setOutputFolder("test");

  cc.workspace.addPlatform("x86", EPlatformTypeX86);
  cc.workspace.addPlatform("x64", EPlatformTypeX64);
  cc.workspace.addConfiguration("Debug");
  cc.workspace.addConfiguration("Release");

  auto p                 = cc.project.create("hello_world", CCProjectTypeConsoleApplication);
  const char* srcFiles[] = {"src/main.c", "src/function.c", NULL};
  for (auto f : srcFiles) {
    if (f) cc.project.addFile(p, f, "Source Files");
  }
  cc.workspace.addProject(p);

  cc.generateInFolder("build");

  return 0;
}