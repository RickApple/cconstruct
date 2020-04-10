#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("bin/x64");
  cc.workspace.setLabel("library_dependency");

  CCPlatformHandle platform_x86 = cc.createPlatform(EPlatformTypeX86);
  CCPlatformHandle platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x86);
  cc.workspace.addPlatform(platform_x64);

  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  void* l = cc.createProject("my_library", CCProjectTypeStaticLibrary);
  {
    const char* files[] = {"src/library/library.c", "src/library/function.h"};
    cc.project.addFiles(l, "Source Files", countof(files), files);
  }

  void* b = cc.createProject("my_binary", CCProjectTypeConsoleApplication);
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, "Source Files", countof(files), files);
    cc.project.addInputProject(b, l);
  }

#if defined(_MSC_VER)
  cc_default_generator("build/msvc");
#else
  cc_default_generator("build/xcode");
#endif

  return 0;
}