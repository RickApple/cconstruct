// This include isn't needed for compilation, but does help for auto-complete while editing the
// file.
#include "../../../source/cconstruct.h"

void add_function(cconstruct_t cc, cc_project_t p) {
  const char* files[] = {"function.c", "function.h"};
  cc.project.addFilesFromFolder(p, folder_path_only(__FILE__), countof(files), files, NULL);
}