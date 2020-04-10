#include <stdio.h>

int main(int argc, const char* argv[]) {
  // TEST_VALUE is defined in the config file
  printf("%i\n", TEST_VALUE);
  return (TEST_VALUE != 5);
}
