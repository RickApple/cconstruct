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

char *append_string(char *destination, const char *source)
{
    //TODO: add safety
    size_t length = strlen(source);
    strcpy(destination, source);
    return destination + length;
}

char *append_printf(char *destination, const char *fmt, ...)
{
    //TODO: add safety
    va_list args;
    va_start(args, fmt);
    int length = vsprintf(destination, fmt, args);
    va_end(args);

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
