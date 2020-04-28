#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");

  cc_platform_t platform           = cc.createPlatform(EPlatformTypeX64);
  cc_configuration_t configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  cc_project_t p = cc.createProject("other_file_types", CCProjectTypeConsoleApplication, NULL);

  cc_group_t g        = cc.createGroup("Source", NULL);
  const char* files[] = {"src/main.c", "src/function.h", "src/function.inl"};
  cc.project.addFiles(p, countof(files), files, g);
  const char* readme[] = {"readme.md"};
  cc.project.addFiles(p, 1, readme, NULL);

  // Folder to generate projects in is also relative to main CConstruct config file.
  cc_default_generator("build");

  return 0;
}