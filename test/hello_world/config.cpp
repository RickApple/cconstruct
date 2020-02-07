#include <vector>
#include <stdio.h>

#include "../../source/cconstruct.h"

int main()
{
  auto builder = cc_xcode_builder;
  builder.workspace.setOutputFolder("test");

  builder.workspace.addPlatform("Win32", EPlatformTypeX86);
  builder.workspace.addPlatform("x64", EPlatformTypeX64);
  builder.workspace.addConfiguration("Debug");
  builder.workspace.addConfiguration("Release");

  auto p = builder.project.create("hello_world", CCProjectTypeConsoleApplication);
  const char *srcFiles[] = {"src/main.c", "src/function.c", NULL};
  for (auto f : srcFiles)
  {
    if (f)
      builder.project.addFile(p, f, "Source Files");
  }
  builder.workspace.addProject(p);
  builder.generate();

  return 0;
}