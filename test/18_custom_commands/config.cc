#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc.workspace.setOutputFolder("${platform}/${configuration}");

  cc_platform_t platform           = cc.createPlatform(EPlatformTypeX64);
  cc_configuration_t configuration = cc.createConfiguration("Debug");

  cc.workspace.addPlatform(platform);
  cc.workspace.addConfiguration(configuration);

  cc_project_t p = cc.createProject("custom_commands", CCProjectTypeConsoleApplication, NULL);

  cc_group_t g        = cc.createGroup("Source", NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, g);

  // Want to add a custom copy command for src/test.txt
  const char* input_file_path  = "src/test_source.txt";
  const char* output_file_path = "src/test.txt";
#if defined(_WIN32)
  cc.project.addFileWithCustomCommand(p, input_file_path, g,
                                      "copy src\\test_source.txt src\\test.txt", output_file_path);
#else
  cc.project.addFileWithCustomCommand(p, custom_compile_command_file, g,
                                      "cp ../src/test_source.txt ../src/test.txt",
                                      "../src/test.txt");
#endif

#if defined(_WIN32)
  cc_state_t flags = cc.createState();
  cc.state.reset(flags);
  cc.state.addPreprocessorDefine(flags, "_CRT_SECURE_NO_WARNINGS");
  cc.project.setFlags(p, flags, NULL, NULL);
#endif

  cc_default_generator("build");

  return 0;
}