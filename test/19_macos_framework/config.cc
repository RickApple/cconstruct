#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setLabel("macos_framework");

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug = cc.configuration.create("Debug");
  cc.workspace.addConfiguration(configuration_debug);

  cc_project_t b = cc.project.create("my_binary", CCProjectTypeConsoleApplication, NULL);
  {
    const char* files[] = {"src/main.m"};
    cc.project.addFiles(b, countof(files), files, NULL);

    cc_state_t s = cc.state.create();
    cc.state.linkExternalLibrary(s, "System/Library/Frameworks/Metal.framework");
    cc.project.setFlags(b, s, NULL, NULL);
  }

#if defined(__APPLE__)
  cc.generator.standard("build");
#endif

  return 0;
}