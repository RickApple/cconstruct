#include "../../source/cconstruct.h"

int main() {
  cc.workspace.setOutputFolder("${platform}/${configuration}");
  cc.workspace.setLabel("project_structure");

  CCPlatformHandle platform_x64 = cc.createPlatform(EPlatformTypeX64);
  cc.workspace.addPlatform(platform_x64);

  CCConfigurationHandle configuration_debug   = cc.createConfiguration("Debug");
  CCConfigurationHandle configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  const cc_group_t group_binaries        = cc.createGroup("Binaries", NULL);
  const cc_group_t group_binaries_nested = cc.createGroup("Nested", group_binaries);
  const cc_group_t group_libraries       = cc.createGroup("Libraries", NULL);

  void* l = cc.createProject("my_library", CCProjectTypeStaticLibrary, group_libraries);
  {
    const char* c_files[] = {"src/library/library.c"};
    const cc_group_t sfg  = cc.createGroup("Source Files", NULL);
    cc.project.addFiles(l, countof(c_files), c_files, cc.createGroup("Nested", sfg));
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(l, countof(h_files), h_files, cc.createGroup("Header Files", NULL));
  }

  void* b = cc.createProject("my_binary", CCProjectTypeConsoleApplication, group_binaries_nested);
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
    cc.project.addInputProject(b, l);
  }

  cc_default_generator("build");

  return 0;
}