class C {
 public:
  int i;
  float f;
};

extern "C" int cFunction();

int main(int argc, const char* argv[]) {
  C c;
  c.i = cFunction();
  return c.i;
}
