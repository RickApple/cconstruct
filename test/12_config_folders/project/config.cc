#include "../../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p      = cc.project.create("config_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"../src/main.c", "../src/function/function.h",
                         "../src/function/function.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  cc.state.addIncludeFolder(flags, "../src/function");
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generateInFolder("../build");

  return 0;
}