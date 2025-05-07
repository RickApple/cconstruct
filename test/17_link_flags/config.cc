#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug   = cc.configuration.create("Debug");
  cc_configuration_t configuration_release = cc.configuration.create("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t p = cc.project.create("link_flags", CCProjectTypeConsoleApplication, NULL);

  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc_state_t flags = cc.state.create();
#ifdef WIN32
  // Note that some linker flags are integrated into the IDE, and other are simply forwarded to the
  // Other Flags section.
  cc.state.addLinkerFlag(flags, "/DEBUG /PDB:${workspace_folder}/${platform}/${configuration}/link_flags_named.pdb");
#else
#endif
  cc.project.setFlags(p, flags, NULL, NULL);

  cc.generator.standard("build");

  return 0;
}