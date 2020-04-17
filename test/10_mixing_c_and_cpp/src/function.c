struct S {
  int i;
  float f;
};

int cFunction() {
  struct S s = {.i = 0, .f = 1.f};
  return s.i;
}
