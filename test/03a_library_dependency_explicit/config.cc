#include "../../source/cconstruct.h"

#define OUTPUT_FOLDER "build"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addPlatform(platform);
  cc_architecture_t arch64 = cc.architecture.create(EArchitectureX64);
  cc.workspace.addArchitecture(arch64);
#if defined(_WIN32)
  cc_architecture_t arch32 = cc.architecture.create(EArchitectureX86);
  cc.workspace.addArchitecture(arch32);
#endif

  cc_configuration_t configuration_debug   = cc.configuration.create("Debug");
  cc_configuration_t configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  // Describe binary
  cc_project_t b = cc.project.create("my_binary", CCProjectTypeConsoleApplication, NULL);
  cc.project.setOutputFolder(b, "${platform}/${configuration}/bin");
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
  }
  {
    cc_state_t s = cc.state.create();
#if defined(_WIN32)
    cc.state.linkExternalLibrary(s,
                                 OUTPUT_FOLDER "/${platform}/${configuration}/lib/my_library.lib");
#else
    cc.state.linkExternalLibrary(s, OUTPUT_FOLDER "/${platform}/${configuration}/lib/my_library");
#endif
    cc.project.setFlags(b, s, NULL, NULL);
  }

  // Describe library
  cc_project_t l = cc.project.create("my_library", CCProjectTypeStaticLibrary, NULL);
  cc.project.setOutputFolder(l, "${platform}/${configuration}/lib");
  {
    const char* c_files[] = {"src/library/library.c"};
    cc.project.addFiles(l, countof(c_files), c_files, NULL);
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(l, countof(h_files), h_files, NULL);
  }
  {
    cc_state_t s = cc.state.create();
    cc.state.addPreprocessorDefine(s, "IS_DEBUG=1");
    cc.project.setFlags(l, s, NULL, configuration_debug);
  }

  // The order in which projects are created determines the default build order.
  // Because this example explicitly links the library files, instead of adding reference,
  // the build would fail because the binary builds before the library.
  //
  // Adding an explicit build order between the two projects solves this.
  cc.project.setBuildOrder(l, b);

  cc.generateInFolder(OUTPUT_FOLDER);

  return 0;
}