#include <iostream>

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
  std::cout << "C++ file output" << std::endl;
  return c.i;
}
