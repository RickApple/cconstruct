#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const char* read_file(const char* file_path) {
  FILE* f = fopen(file_path, "rb");
  if (f == NULL) return NULL;

  fseek(f, 0, SEEK_END);
  unsigned size = (unsigned)ftell(f);
  fseek(f, 0, SEEK_SET);
  char* out = malloc(size + 1);
  fread(out, 1, size, f);
  out[size] = 0;
  fclose(f);
  return out;
}

int main() {
  const char* contents = read_file("src/test.txt");
  printf("test %s\n", contents);
  free((void*)contents);
  return 0;
}
