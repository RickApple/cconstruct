#include "../../source/cconstruct.h"

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_architecture_t arch       = cc.architecture.create(EArchitectureX64);
  cc_architecture_t platform   = cc.platform.create(EPlatformPhone);
  cc.workspace.addPlatform(platform);
  cc.workspace.addArchitecture(arch);

  cc_project_t p = cc.project.create("TestGame", CCProjectTypeWindowedApplication, NULL);

  cc_group_t g = cc.group.create("Source", NULL);
  {
    const char* files[] = {"main.m",
                           "AppDelegate.h",
                           "AppDelegate.m",
                           "GameViewController.h",
                           "GameViewController.m",
                           "Renderer.h",
                           "Renderer.m",
                           "Shaders.metal",
                           "ShaderTypes.h",
                           "Info.plist"};
    cc.project.addFilesFromFolder(p, "src/", countof(files), files, g);
  }
  {
    const char* files[] = {"Main.storyboard", "LaunchScreen.storyboard"};
    cc.project.addFilesFromFolder(p, "src/Base.lproj/", countof(files), files, g);
  }
  {
    const char* files[] = {"Assets.xcassets"};
    cc.project.addFilesFromFolder(p, "src/", countof(files), files, g);
  }
#if defined(__APPLE__)
  cc_state_t flags = cc.state.create();
  cc.state.disableWarningsAsErrors(flags);
  cc.project.setFlags(p, flags, NULL, NULL);
  cc.generateInFolder("build");
#endif

  return 0;
}