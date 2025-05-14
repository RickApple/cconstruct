#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setLabel("library_dependency");

  cc_project_t ls = cc.project.create("my_library", CCProjectTypeStaticLibrary, NULL);
  {
    const char* c_files[] = {"src/library/library.c"};
    cc.project.addFiles(ls, countof(c_files), c_files, NULL);
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(ls, countof(h_files), h_files, NULL);
  }

  cc_project_t ld = cc.project.create("my_dynamic_library", CCProjectTypeDynamicLibrary, NULL);
  {
    const char* c_files[] = {"src/library/library_dynamic.c"};
    cc.project.addFiles(ld, countof(c_files), c_files, NULL);
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(ld, countof(h_files), h_files, NULL);
  }

  cc_project_t b = cc.project.create("my_binary", CCProjectTypeConsoleApplication, NULL);
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
    cc.project.addInputProject(b, ls);
    cc.project.addInputProject(b, ld);
  }

  cc.generateInFolder("build");

  return 0;
}