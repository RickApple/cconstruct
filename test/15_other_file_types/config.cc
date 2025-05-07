#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration = cc.configuration.create("Debug");
  cc.workspace.addConfiguration(configuration);

  cc_project_t p = cc.project.create("other_file_types", CCProjectTypeConsoleApplication, NULL);

  cc_group_t g        = cc.group.create("Source", NULL);
  const char* files[] = {"src/main.c", "src/function.h", "src/function.inl"};
  cc.project.addFiles(p, countof(files), files, g);
  const char* readme[] = {"readme.md"};
  cc.project.addFiles(p, 1, readme, NULL);

  cc.generator.standard("build");

  return 0;
}