#include "../../source/cconstruct.h"

int main()
{
  auto cc = cc_xcode_builder;
  cc.workspace.setOutputFolder("build/bin/x64");
  cc.workspace.setLabel("library_dependency");

  cc.workspace.addPlatform("Win32", EPlatformTypeX86);
  cc.workspace.addPlatform("x64", EPlatformTypeX64);
  cc.workspace.addConfiguration("Debug");
  cc.workspace.addConfiguration("Release");

  auto l = cc.project.create("my_library", CCProjectTypeStaticLibrary);
  cc.project.addFile(l, "src/library.c", "Source Files");

  auto b = cc.project.create("my_binary", CCProjectTypeConsoleApplication);
  cc.project.addFile(b, "src/main.c", "Source Files");
  cc.project.addInputProject(b, l);

  cc.workspace.addProject(l);
  cc.workspace.addProject(b);

  cc.generate();

  return 0;
}