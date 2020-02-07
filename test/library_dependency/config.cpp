#include <vector>
#include <stdio.h>

#include "../../source/cconstruct.h"

int main()
{
  auto builder = cc_xcode_builder;
  builder.workspace.setOutputFolder("test");
  builder.workspace.setLabel("library_dependency");

  builder.workspace.addPlatform("Win32", EPlatformTypeX86);
  builder.workspace.addPlatform("x64", EPlatformTypeX64);
  builder.workspace.addConfiguration("Debug");
  builder.workspace.addConfiguration("Release");

  auto l = builder.project.create("my_library", CCProjectTypeStaticLibrary);
  builder.project.addFile(l, "src/library.c", "Source Files");

  auto b = builder.project.create("my_binary", CCProjectTypeConsoleApplication);
  builder.project.addFile(b, "src/main.c", "Source Files");

  builder.workspace.addProject(l);
  builder.workspace.addProject(b);

  builder.generate();

  return 0;
}