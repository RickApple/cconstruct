#include <stdio.h>

int main() {
  // TEST_VALUE is defined in the config file
  printf("Defined value: %i, test value: 5\n", TEST_VALUE);
  return (TEST_VALUE != 5);
}
