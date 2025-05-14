#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p = cc.project.create("mixing_c_and_cpp", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.cpp", "src/function.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t f = cc.state.create();
  cc.state.setCppVersion(f, 11);
  
  #if defined(_WIN32)
  cc.state.addCompilerFlag(f, "/EHsc");
  #endif
  
  cc.project.setFlags(p, f, NULL, NULL);

  cc.generateInFolder("build");

  return 0;
}