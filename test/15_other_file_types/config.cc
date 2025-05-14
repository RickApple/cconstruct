#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p = cc.project.create("other_file_types", CCProjectTypeConsoleApplication, NULL);

  cc_group_t g        = cc.group.create("Source", NULL);
  const char* files[] = {"src/main.c", "src/function.h", "src/function.inl"};
  cc.project.addFiles(p, countof(files), files, g);
  const char* readme[] = {"readme.md"};
  cc.project.addFiles(p, 1, readme, NULL);

  cc.generateInFolder("build");

  return 0;
}