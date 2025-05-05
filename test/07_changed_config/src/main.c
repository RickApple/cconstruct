#include <stdio.h>

int main() {
  // TEST_VALUE is defined in the config file
  printf("return code: %i\n", RETURN_CODE);
  return RETURN_CODE;
}
