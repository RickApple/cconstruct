#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p      = cc.project.create("compile_flags", CCProjectTypeConsoleApplication, NULL);
  const char* files[] = {"src/main.c"};

  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
  // Beware, there are more general ways to set warning flags, see other test case.
#ifdef WIN32
  cc.state.addCompilerFlag(flags, "/WX");
  cc.state.addCompilerFlag(flags, "/Wall");
#else
  cc.state.addCompilerFlag(flags, "-Werror");
  cc.state.addCompilerFlag(flags, "-Wunused-variable");
  if (cc.generateInFolder == cc.generator.xcode) {
    cc.state.addCompilerFlag(flags, "MACOSX_DEPLOYMENT_TARGET=10.15");
  }
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generateInFolder("build");

  return 0;
}