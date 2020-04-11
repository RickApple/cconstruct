#include <stdio.h>

int main(int argc, const char* argv[]) {
  // TEST_VALUE is defined in the config file
  printf("return code: %i\n", RETURN_CODE);
  return RETURN_CODE;
}
