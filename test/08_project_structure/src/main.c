#include <string.h>

#include "library/function.h"

int main(int argc, const char* argv[]) {
  const char* lib_string = getLibraryString();
  return strcmp(lib_string, "Hello from library");
}
