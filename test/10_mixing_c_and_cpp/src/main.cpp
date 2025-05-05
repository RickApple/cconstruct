class C {
 public:
  int i;
  float f;
};

extern "C" int cFunction();

int main() {
  C c;
  c.i = cFunction();
  return c.i;
}
