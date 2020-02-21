#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("bin/x64");
  cc.workspace.setLabel("library_dependency");

  auto platform_x86 = cc.platform.create("Win32", EPlatformTypeX86);
  auto platform_x64 = cc.platform.create("x64", EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x86);
  cc.workspace.addPlatform(platform_x64);

  auto configuration_debug   = cc.configuration.create("Debug");
  auto configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

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

#if defined(_MSC_VER)
  cc_default_generator("build/msvc");
#else
  cc_default_generator("build/xcode");
#endif

  return 0;
}