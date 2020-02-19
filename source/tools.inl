#if defined(_MSC_VER)
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string replaceSpacesWithUnderscores(std::string in) {
  std::replace(in.begin(), in.end(), ' ', '_');
  return in;
}

const char* strip_path(const char* path) { return strrchr(path, '/') + 1; }

char* append_string(char* destination, const char* source) {
  // TODO: add safety
  size_t length = strlen(source);
  strcpy(destination, source);
  return destination + length;
}

#if defined(_MSC_VER)
#include <direct.h>

int make_folder(const char* folder_path) {
  char buffer[1024] = {0};

  const char* next_sep = folder_path;
  while ((next_sep = strchr(next_sep, '/')) != NULL) {
    strncpy(buffer, folder_path, next_sep - folder_path);
    int result = _mkdir(buffer);
    if (result != 0) {
      if (errno != EEXIST) return errno;
    }
    next_sep += 1;
  }
  int result = _mkdir(folder_path);
  if (result != 0) {
    if (errno != EEXIST) return errno;
  }
  return 0;
}
#else
int make_folder(const char* folder_path) {
  char buffer[1024] = {0};

  const char* next_sep = folder_path;
  while ((next_sep = strchr(next_sep, '/')) != NULL) {
    strncpy(buffer, folder_path, next_sep - folder_path);
    int result = mkdir(buffer, 0777);
    if (result != 0) {
      if (errno != EEXIST) return errno;
    }
    next_sep += 1;
  }
  int result = mkdir(folder_path, 0777);
  if (result != 0) {
    if (errno != EEXIST) return errno;
  }
  return 0;
}
#endif

void* cc_alloc_(size_t size) {
  static uintptr_t next_free     = NULL;
  static uintptr_t end_next_free = next_free;

  if ((end_next_free - next_free) < size) {
    // Doesn't fit, allocate a new block
    size_t byte_count = 1024 * 1024;
    if (size > byte_count) byte_count = size;
    next_free     = (uintptr_t)malloc(byte_count);
    end_next_free = next_free + byte_count;
  }

  void* out = (void*)next_free;
  next_free += size;
  return out;
}