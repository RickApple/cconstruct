#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p = cc.project.create("hello_world", CCProjectTypeConsoleApplication, NULL);
  // Files are added relative to the main CConstruct config file.
  const char* files[] = {"src/main.c", "src/function.c", "src/function.h"};
  cc.project.addFiles(p, countof(files), files, NULL);

  // Folder to generate projects in is also relative to main CConstruct config file.
  cc.generateInFolder("build");

  return 0;
}