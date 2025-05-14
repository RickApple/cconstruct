#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p =
      cc.project.create("windowed_application", CCProjectTypeWindowedApplication, NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.generateInFolder("build");

  return 0;
}