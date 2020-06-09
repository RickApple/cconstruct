#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setLabel("macos_framework");

  cc_architecture_t arch = cc.createArchitecture(EArchitectureX64);
  cc_platform_t platform = cc.createPlatform(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug = cc.createConfiguration("Debug");
  cc.workspace.addConfiguration(configuration_debug);

  cc_project_t b = cc.createProject("my_binary", CCProjectTypeConsoleApplication, NULL);
  {
    const char* files[] = {"src/main.m"};
    cc.project.addFiles(b, countof(files), files, NULL);

    cc_state_t s = cc.createState();
    cc.state.linkExternalLibrary(s, "System/Library/Frameworks/Metal.framework");
    cc.project.setFlags(b, s, NULL, NULL);
  }

#if defined(__APPLE__)
  cc_default_generator("build/xcode");
#endif

  return 0;
}