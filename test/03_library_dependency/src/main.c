#include <string.h>

#include "library/function.h"

int main(int argc, const char* argv[]) {
  const char* lib_string    = getLibraryString();
  const char* dynlib_string = getDynamicLibraryString();
  return !((strcmp(lib_string, "Hello from library") == 0) &&
           (strcmp(dynlib_string, "Hello from dynamic library") == 0));
}
