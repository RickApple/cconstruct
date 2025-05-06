#include <stdio.h>
#include <string.h>

#include "library/function.h"

int main() {
  const char* lib_string    = getLibraryString();
  const char* dynlib_string = getDynamicLibraryString();
  printf("Running binary with dynamic library\n");
  return !((strcmp(lib_string, "Hello from library") == 0) &&
           (strcmp(dynlib_string, "Hello from dynamic library") == 0));
}
