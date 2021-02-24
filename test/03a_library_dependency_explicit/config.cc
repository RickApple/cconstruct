#include "../../source/cconstruct.h"


#if defined(_MSC_VER)
  #define OUTPUT_FOLDER "build/msvc"
#else
  #define OUTPUT_FOLDER "build/xcode"
#endif


int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setLabel("library_dependency_explicit");

  cc_platform_t platform = cc.createPlatform(EPlatformDesktop);
  cc.workspace.addPlatform(platform);
  cc_architecture_t arch64 = cc.createArchitecture(EArchitectureX64);
  cc.workspace.addArchitecture(arch64);
#if defined(_WIN32)
  cc_architecture_t arch32 = cc.createArchitecture(EArchitectureX86);
  cc.workspace.addArchitecture(arch32);
#endif

  cc_configuration_t configuration_debug   = cc.createConfiguration("Debug");
  cc_configuration_t configuration_release = cc.createConfiguration("Release");
  cc.workspace.addConfiguration(configuration_debug);
  cc.workspace.addConfiguration(configuration_release);

  cc_project_t l = cc.createProject("my_library", CCProjectTypeStaticLibrary, NULL);
  cc.project.setOutputFolder(l, "${platform}/${configuration}/lib");
  {
    const char* c_files[] = {"src/library/library.c"};
    cc.project.addFiles(l, countof(c_files), c_files, NULL);
    const char* h_files[] = {"src/library/function.h"};
    cc.project.addFiles(l, countof(h_files), h_files, NULL);
  }
  {
    cc_state_t s = cc.createState();
    cc.state.addPreprocessorDefine(s, "IS_DEBUG=1");
    cc.project.setFlags(l, s, NULL, configuration_debug);
  }

  cc_project_t b = cc.createProject("my_binary", CCProjectTypeConsoleApplication, NULL);
  cc.project.setOutputFolder(b, "${platform}/${configuration}/bin");
  {
    const char* files[] = {"src/main.c"};
    cc.project.addFiles(b, countof(files), files, NULL);
  }

#if defined(_WIN32)
  {
    cc_state_t s = cc.createState();
    cc.state.linkExternalLibrary(s, OUTPUT_FOLDER "/${platform}/Debug/lib/my_library.lib");
    cc.project.setFlags(b, s, NULL, configuration_debug);
  }
  {
    cc_state_t s = cc.createState();
    cc.state.linkExternalLibrary(s, OUTPUT_FOLDER "/${platform}/Release/lib/my_library.lib");
    cc.project.setFlags(b, s, NULL, configuration_release);
  }
#else
  {
    cc_state_t s = cc.createState();
    cc.state.linkExternalLibrary(s, OUTPUT_FOLDER "/${platform}/Debug/lib/my_library");
    cc.project.setFlags(b, s, NULL, configuration_debug);
  }
  {
    cc_state_t s = cc.createState();
    cc.state.linkExternalLibrary(s, OUTPUT_FOLDER "/${platform}/Release/lib/my_library");
    cc.project.setFlags(b, s, NULL, configuration_release);
  }
#endif

  cc_default_generator(OUTPUT_FOLDER);

  return 0;
}