#include "../../source/cconstruct.h"

int main() {
  auto cc = cc_default;

  cc.workspace.setOutputFolder("bin/x64");
  cc.workspace.setLabel("library_dependency");

  auto platform_x86 = cc.platform.create("Win32", EPlatformTypeX86);
  auto platform_x64 = cc.platform.create("x64", EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x86);
  cc.workspace.addPlatform(platform_x64);

  cc.workspace.addConfiguration("Debug");
  cc.workspace.addConfiguration("Release");

  auto l = cc.project.create("my_library", CCProjectTypeStaticLibrary);
  {
    const char* files[] = {"src/library.c", NULL};
    cc.project.addFiles(l, "Source Files", files);
  }

  auto b = cc.project.create("my_binary", CCProjectTypeConsoleApplication);
  {
    const char* files[] = {"src/main.c", NULL};
    cc.project.addFiles(b, "Source Files", files);
    cc.project.addInputProject(b, l);
  }

  cc.workspace.addProject(l);
  cc.workspace.addProject(b);

#if defined(_MSC_VER)
  cc.generateInFolder("build/msvc");
#else
  cc.generateInFolder("build/xcode");
#endif

  return 0;
}