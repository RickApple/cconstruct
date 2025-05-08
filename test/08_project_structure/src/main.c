#include <string.h>

#include "library/function.h"

int main() {
  const char* lib_string = getLibraryString();
  return strcmp(lib_string, "Hello from library");
}
