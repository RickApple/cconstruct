#include <stdio.h>

// Returns a different value depending on architecture that was used to compile CConstruct
int main() {
  printf("Size of void*: %zu\n", sizeof(void*));
  return OUTPUT;
}
