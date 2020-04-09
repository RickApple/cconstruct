#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../source/tools.inl"

const char* combinePaths(const char* folder1, const char* folder2) {
  unsigned combined_length = strlen(folder1) + 1 + strlen(folder2) + 1;
  char* out_buffer         = (char*)cc_alloc_(combined_length);
  char* append_buffer      = out_buffer;
  strcpy(out_buffer, folder1);

  append_buffer += strlen(folder1);

  if (append_buffer[-1] != '/') {
    *append_buffer = '/';
    append_buffer += 1;
  }

  strcpy(append_buffer, folder2);
  append_buffer += strlen(folder2);
  *append_buffer = 0;

  return out_buffer;
}

int copyFileContents(const char* in_file_path, FILE* out_file) {
  FILE* in_file = fopen(in_file_path, "rb");
  if (in_file == NULL) {
    fprintf(stderr, "Couldn't open include file '%s'\n", in_file_path);
    return 1;
  }

  long in_size = 0;

  fseek(in_file, 0, SEEK_END);
  in_size = ftell(in_file);
  fseek(in_file, 0, SEEK_SET);

  char* in_data = (char*)cc_alloc_(in_size);
  fread(in_data, 1, in_size, in_file);
  fclose(in_file);

  fwrite(in_data, 1, in_size, out_file);

  return 0;
}

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Input format is 'cconstruct_release input_file_path output_file_path");
    return 1;
  }

  printf("Building '%s' to '%s'\n", argv[1], argv[2]);
  // TODO: ensure folders exist

  const char* in_file_path  = argv[1];
  const char* out_file_path = argv[2];
  FILE* in_file             = fopen(in_file_path, "rb");
  FILE* out_file            = fopen(out_file_path, "wb");

  if (in_file == NULL) {
    fprintf(stderr, "Couldn't open input file '%s'\n", in_file_path);
  }
  if (out_file == NULL) {
    fprintf(stderr, "Couldn't open output file '%s'\n", out_file_path);
  }
  if (in_file == NULL || out_file == NULL) {
    return 1;
  }

  char* in_data = NULL;
  long in_size  = 0;
  {
    fseek(in_file, 0, SEEK_END);
    in_size = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);

    in_data = (char*)malloc(in_size + 1);
    fread(in_data, 1, in_size, in_file);
    in_data[in_size] = 0;
    fclose(in_file);
  }

  const char* in_file_only  = strip_path(in_file_path);
  char in_folder_path[2048] = {0};
  memcpy(in_folder_path, in_file_path, in_file_only - in_file_path);
  printf("root path '%s'\n", in_folder_path);

  char* next_include                  = NULL;
  const char* k_include_search        = "#include \"";
  char include_file_path_buffer[2048] = {0};

  while ((next_include = strstr(in_data, k_include_search)) != NULL) {
    // Write out string before the include
    fwrite(in_data, 1, next_include - in_data, out_file);

    in_data           = next_include + strlen(k_include_search);
    char* end_include = strchr(in_data, '"');
    memcpy(include_file_path_buffer, in_data, end_include - in_data);
    include_file_path_buffer[end_include - in_data] = 0;

    in_data = end_include + 1;

    const char* include_file_path = combinePaths(in_folder_path, include_file_path_buffer);
    printf("Including '%s' (%s)\n", include_file_path_buffer, include_file_path);

    // TODO: recursive includes
    copyFileContents(include_file_path, out_file);
  }

  // Write out remainder of input file
  fwrite(in_data, 1, strlen(in_data), out_file);

  fclose(out_file);

  printf("Finished building '%s' to '%s'\n", in_file_path, out_file_path);

  return 0;
}
