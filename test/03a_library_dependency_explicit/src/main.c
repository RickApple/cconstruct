#include <stdio.h>

extern int valueFromLib();

int main() {
  int val = valueFromLib();
  printf("Dependency print: %i\n", val);
  return val;
}
