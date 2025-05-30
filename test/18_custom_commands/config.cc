#include "../../source/cconstruct.h"

struct C_struct {
  const char* debug_name;
  const char* release_name;
};

int main(int argc, const char** argv) {
  cconstruct_t cc = cc_init(__FILE__, argc, argv);

  cc_project_t p = cc.project.create("custom_commands", CCProjectTypeConsoleApplication, NULL);

  cc_group_t g        = cc.group.create("Source", NULL);
  const char* files[] = {"src/main.c"};
  cc.project.addFiles(p, countof(files), files, g);

  // Want to add a custom copy command for src/test.txt
  // TODO: something that uses more than a single input file
  // TODO: pre/post actions
  const char* input_file_path  = "src/test_source.txt";
  const char* output_file_path = "src/test.txt";
#if defined(_WIN32)
  cc.project.addFileWithCustomCommand(p, input_file_path, g, "copy ${input} ${output}",
                                      output_file_path);
#else
  cc.project.addFileWithCustomCommand(p, input_file_path, g, "cp ${input} ${output}",
                                      output_file_path);
#endif

#if defined(_WIN32)
  cc_state_t flags = cc.state.create();
  cc.state.reset(flags);
  cc.state.addPreprocessorDefine(flags, "_CRT_SECURE_NO_WARNINGS");
  cc.project.setFlags(p, flags, NULL, NULL);
#endif

  cc.generateInFolder("build");

  return 0;
}