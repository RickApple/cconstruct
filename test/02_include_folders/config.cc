#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p      = cc.project.create("include_folders", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c", "src/function.c", "src/include/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  // Include folders are assumed to be relative to main CConstruct config file
  cc.state.addIncludeFolder(flags, "src/include");
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generateInFolder("build");

  return 0;
}