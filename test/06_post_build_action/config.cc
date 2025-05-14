#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p = cc.project.create("post_build_action", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);
  cc.project.addPostBuildAction(
      p, "./bin/test");  // Note that this command will fail, as required by the test runner

  cc.generateInFolder("build");

  return 0;
}