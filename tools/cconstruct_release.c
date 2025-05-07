#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
  // First
  #include <Windows.h>
  // Later
  #include <direct.h>
#else
  #include <errno.h>
  #include <sys/stat.h>
#endif

#include "../source/cconstruct.h"

const char* combinePaths(const char* folder1, const char* folder2) {
  unsigned combined_length = (unsigned)(strlen(folder1) + 1 + strlen(folder2) + 1);
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

char* readFileContents(const char* in_file_path) {
  FILE* in_file = fopen(in_file_path, "rb");
  if (in_file == NULL) {
    fprintf(stderr, "Couldn't open include file '%s'\n", in_file_path);
    return NULL;
  }

  size_t in_size = 0;

  fseek(in_file, 0, SEEK_END);
  in_size = (size_t)ftell(in_file);
  fseek(in_file, 0, SEEK_SET);

  char* in_data = (char*)cc_alloc_((unsigned)in_size + 1);
  fread(in_data, 1, in_size, in_file);
  in_data[in_size] = 0;
  fclose(in_file);

  return in_data;
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
  FILE* out_file            = fopen(out_file_path, "wb");

  char* in_data = readFileContents(in_file_path);

  if (in_data == NULL) {
    fprintf(stderr, "Couldn't open input file '%s'\n", in_file_path);
  }
  if (out_file == NULL) {
    fprintf(stderr, "Couldn't open output file '%s'\n", out_file_path);
  }
  if (in_data == NULL || out_file == NULL) {
    return 1;
  }

  const char* in_file_only  = strip_path(in_file_path);
  char in_folder_path[2048] = {0};
  memcpy(in_folder_path, in_file_path, (size_t)(in_file_only - in_file_path));
  printf("root path '%s'\n", in_folder_path);

  char* next_include           = NULL;
  const char* k_include_search = "#include \"";

  while ((next_include = strstr(in_data, k_include_search)) != NULL) {
    // Get path to file
    const char* start_include = next_include + strlen(k_include_search);
    char* end_include         = strchr(start_include, '"');
    *end_include              = 0;
    const char* remainder     = end_include + 1;

    *next_include = 0;

    const char* include_file_path = combinePaths(in_folder_path, start_include);
    printf("Including '%s' (%s)\n", start_include, include_file_path);

    const char* included_file_data = readFileContents(include_file_path);

    // This can be edited for sure, so cast is safe
    size_t pre_len       = strlen(in_data);
    size_t include_len   = strlen(included_file_data);
    size_t remainder_len = strlen(remainder);
    char* new_file       = (char*)cc_alloc_(pre_len + include_len + remainder_len + 1);
    memcpy(new_file, in_data, pre_len);
    memcpy(new_file + pre_len, included_file_data, include_len);
    memcpy(new_file + pre_len + include_len, remainder, remainder_len);
    new_file[pre_len + include_len + remainder_len] = 0;
    in_data                                         = new_file;
  }

  // Write the output file
  fwrite(in_data, 1, strlen(in_data), out_file);

  fclose(out_file);

  printf("Finished building '%s' to '%s'\n", in_file_path, out_file_path);

  return 0;
}
