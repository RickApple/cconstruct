#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setLabel("project_structure");

  const cc_group_t group_binaries        = cc.group.create("Binaries", NULL);
  const cc_group_t group_binaries_nested = cc.group.create("Nested", group_binaries);
  const cc_group_t group_libraries       = cc.group.create("Libraries", NULL);

  cc_project_t l = cc.project.create("my_library", CCProjectTypeStaticLibrary, group_libraries);
  {
    const char* c_files[] = {"src/library/library.c"};
    const cc_group_t sfg  = cc.group.create("Source Files", NULL);
    cc.project.addFiles(l, countof(c_files), c_files, cc.group.create("Nested", sfg));
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(l, countof(h_files), h_files, cc.group.create("Header Files", NULL));
  }

  cc_project_t b =
      cc.project.create("my_binary", CCProjectTypeConsoleApplication, group_binaries_nested);
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
    cc.project.addInputProject(b, l);
  }

  cc.generateInFolder("build");

  return 0;
}