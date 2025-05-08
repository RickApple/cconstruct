#include <stdio.h>

class C {
 public:
  int i;
  float f;
};

extern "C" int cFunction();

int main() {
  C c;
  c.i = cFunction();
  // TODO change output to use C++ std::cout
  printf("C file output\n");
  return c.i;
}
