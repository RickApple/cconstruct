#include "../../source/cconstruct.h"

int main(int argc, char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");
  cc.workspace.setLabel("library_dependency");

  cc_platform_t platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t l = cc.createProject("my_library", CCProjectTypeStaticLibrary, NULL);
  {
    const char* c_files[] = {"src/library/library.c"};
    cc.project.addFiles(l, countof(c_files), c_files, NULL);
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(l, countof(h_files), h_files, NULL);
  }

  cc_project_t b = cc.createProject("my_binary", CCProjectTypeConsoleApplication, NULL);
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
    cc.project.addInputProject(b, l);
  }

#if defined(_MSC_VER)
  cc_default_generator("build/msvc");
#else
  cc_default_generator("build/xcode");
#endif

  return 0;
}