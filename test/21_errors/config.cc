#include "../../source/cconstruct.h"
#include "error.inl"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p      = cc.project.create("errors", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  GENERATE_ERROR

  cc.generateInFolder("build");
}