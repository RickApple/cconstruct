#if defined(_MSC_VER)
#include <Windows.h>
#include <direct.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define countof(a) sizeof(a) / sizeof(a[0])

bool cc_is_verbose = false;

#define LOG_ERROR_AND_QUIT(...)   \
  {                               \
    fprintf(stderr, __VA_ARGS__); \
    error_quit();                \
  }
#define LOG_VERBOSE(...) \
  if (cc_is_verbose) fprintf(stdout, __VA_ARGS__)

void error_quit() {
#if defined(_WIN32)
  // The program could have been run crom the command prompt, or by double clicking on a previously
  // built CConstruct executable. In the first case, the output is readily visible. In the second
  // case keep the window open so users can read the error.
  DWORD p[2];
  int count = (int)GetConsoleProcessList((LPDWORD)&p, 2);
  if (count <= 1) {
    // Last one, or error querying, so probably started with a double-click. Should wait for the
    // user to read output.
    system("pause");
  }
#endif

  exit(1);
}

const char* file_extension(const char* file_path) { return strrchr(file_path, '.') + 1; }
bool is_header_file(const char* file_path) { return strstr(file_path, ".h") != 0; }
bool is_source_file(const char* file_path) {
  const char* ext = file_extension(file_path);

  const char* const source_extensions[] = {
      "c",     "cc", "cpp", /* C variant source files*/
      "m",     "mm",        /* Objective-C source files */
      "metal",              /* Apples Metal shading language */
  };
  for (unsigned i = 0; i < countof(source_extensions); i++) {
    if (strcmp(ext, source_extensions[i]) == 0) return true;
  }
  return false;
}
bool is_buildable_resource_file(const char* file_path) {
  const char* ext = file_extension(file_path);

  const char* const resource_extensions[] = {
      "storyboard",
      "xcassets",
  };
  for (unsigned i = 0; i < countof(resource_extensions); i++) {
    if (strcmp(ext, resource_extensions[i]) == 0) return true;
  }
  return false;
}

#if defined(_MSC_VER)

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
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int make_folder(const char* folder_path) {
  char buffer[1024] = {0};

  const char* next_sep = folder_path;
  while ((next_sep = strchr(next_sep, '/')) != NULL) {
    strncpy(buffer, folder_path, (size_t)(next_sep - folder_path));
    if (buffer[0]) {
      int result = mkdir(buffer, 0777);
      if (result != 0) {
        if (errno != EEXIST) return errno;
      }
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

static uintptr_t cc_next_free     = (uintptr_t)NULL;
static uintptr_t cc_end_next_free = (uintptr_t)NULL;

void* cc_alloc_(unsigned size) {
  if ((cc_end_next_free - cc_next_free) < size) {
    // Doesn't fit, allocate a new block
    unsigned byte_count = 1024 * 1024;
    if (size > byte_count) byte_count = size;
    cc_next_free     = (uintptr_t)malloc(byte_count);
    cc_end_next_free = cc_next_free + byte_count;
  }

  void* out = (void*)cc_next_free;
  cc_next_free += size;
  return out;
}

typedef struct array_header_t {
  unsigned count_;
  unsigned capacity_;
} array_header_t;

void* array_grow(void* a, unsigned element_size);

#define array_header(a) ((a) ? ((array_header_t*)((char*)(a) - sizeof(array_header_t))) : 0)

#define array_count(a) ((a) ? array_header(a)->count_ : 0)
#define array_capacity(a) ((a) ? array_header(a)->capacity_ : 0)
#define array_reset(a) ((a) ? array_header(a)->count_ = 0 : 0)
#define array_full(a) ((a) ? (array_header(a)->count_ == array_header(a)->capacity_) : true)

#define array_push(a, item)                                                \
  (array_full(a) ? (*((void**)&a) = array_grow((void*)a, sizeof(*a))) : 0, \
   (a[array_header(a)->count_++] = item))

#define array_remove_at_index(a, idx) \
  memmove(a + idx, a + idx + 1, (array_count(a) - idx - 1) * sizeof(*a)), array_header(a)->count_--

#define array_append(a, data, count)                                                            \
  (((array_count(a) + (count)) > array_capacity(a))                                             \
       ? (*((void**)&(a)) = array_reserve((a), sizeof(*(a)),                                    \
                                          (array_count(a) + (count)) > (array_count(a) * 3 / 2) \
                                              ? (array_count(a) + (count))                      \
                                              : (array_count(a) * 3 / 2)))                      \
       : 0,                                                                                     \
   (memcpy(a + array_count(a), data, ((count) + 0) * sizeof(*a))),                              \
   (array_header(a)->count_ = array_count(a) + (count)))

void* array_grow(void* a, unsigned element_size) {
  unsigned prev_capacity      = 0;
  unsigned prev_count         = 0;
  array_header_t* prev_header = array_header(a);
  if (prev_header) {
    prev_capacity = prev_header->capacity_;
    prev_count    = prev_header->count_;
  }

  unsigned new_capacity = (unsigned)(prev_capacity * 1.75);
  if (new_capacity == prev_capacity) {
    new_capacity = prev_capacity + 8;
  }

  array_header_t* new_header =
      (array_header_t*)cc_alloc_(element_size * new_capacity + sizeof(array_header_t));
  void* out             = new_header + 1;
  new_header->count_    = prev_count;
  new_header->capacity_ = new_capacity;
  memcpy(out, a, element_size * prev_count);

  // At this point could free the prev data, but not doing so in CConstruct

  return out;
}

void* array_reserve(void* a, unsigned element_size, unsigned new_capacity) {
  unsigned prev_capacity      = 0;
  unsigned prev_count         = 0;
  array_header_t* prev_header = array_header(a);
  if (prev_header) {
    prev_capacity = prev_header->capacity_;
    prev_count    = prev_header->count_;
  }

  if (new_capacity > prev_capacity) {
    array_header_t* new_header =
        (array_header_t*)cc_alloc_(element_size * new_capacity + sizeof(array_header_t));
    void* out             = new_header + 1;
    new_header->count_    = prev_count;
    new_header->capacity_ = new_capacity;
    memcpy(out, a, element_size * prev_count);

    // At this point could free the prev data, but not doing so in CConstruct

    return out;
  } else {
    return a;
  }
}

#if !defined(_MSC_VER)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
const char* cc_printf(const char* format, ...) {
  unsigned length = (unsigned)strlen(format);

  // Guess length of output format
  unsigned alloc_size = (2 * length > 256) ? 2 * length : 256;

  char* out = (char*)cc_alloc_(alloc_size);

  va_list args;

  va_start(args, format);
  unsigned output_length = (unsigned)vsnprintf(out, alloc_size - 1, format, args);
  va_end(args);

  if (output_length > alloc_size) {
    alloc_size = output_length + 1;
    out        = (char*)cc_alloc_(alloc_size);
    va_start(args, format);
    vsprintf(out, format, args);
    va_end(args);
  }

  out[alloc_size - 1] = 0;
  return out;
}
#if !defined(_MSC_VER)
#pragma clang diagnostic pop
#endif

const char* cc_substitute(const char* in_original, const char** keys, const char** values,
                          unsigned num_keys) {
  char key_search[128] = {0};

  const char* in = in_original;
  for (unsigned i = 0; i < num_keys; ++i) {
    char* out_string = {0};
    out_string       = (char*)array_reserve(out_string, sizeof(*out_string), 128);

    const char* key = keys[i];
    snprintf(key_search, sizeof(key_search) - 1, "${%s}", key);
    const char* value       = values[i];
    const char* offset      = in;
    const char* piece_start = in;
    while ((offset = strstr(offset, key_search)) != 0) {
      array_append(out_string, piece_start, (unsigned)(offset - piece_start));
      array_append(out_string, value, (unsigned)strlen(value));
      offset      = offset + strlen(key_search);
      piece_start = offset;
    }
    array_append(out_string, piece_start, (unsigned)strlen(piece_start));
    array_push(out_string, 0);
    // Breaking connection to stretchy buffer but that's ok, since we're not tracking memory in
    // CConstruct.
    in = out_string;
  }

  return in;
}

const char** string_array_clone(const char** in) {
  char** out                   = {0};
  const array_header_t* header = array_header(in);
  if (header && header->count_ > 0) {
    array_header_t* out_header =
        (array_header_t*)cc_alloc_(sizeof(array_header_t) + sizeof(const char*) * header->count_);
    out_header->count_ = out_header->capacity_ = header->count_;
    out                                        = (char**)(out_header + 1);
    memcpy(out, in, sizeof(const char*) * header->count_);
  }
  return (const char**)out;
}

/* Strip path information returning only the filename (with extension) */
const char* strip_path(const char* path) {
  const char* last_slash = strrchr(path, '/');
  if (last_slash)
    return last_slash + 1;
  else
    return path;
}

char* make_uri(const char* in_path) {
  char* uri = (char*)cc_printf("%s", in_path);
  char* c   = uri;
  while (*c) {
    if (*c == '\\') {
      *c = '/';
    }
    c++;
  }

  // Remove parent paths, start at offset 3 since can't do anything about a path starting with ../
  if (uri[0] && uri[1] && uri[2]) {
    char* uri_read  = uri + 3;
    char* uri_write = uri + 3;
    while (*uri_read) {
      bool is_parent_path = *uri_read == '/' && *(uri_read + 1) == '.' && *(uri_read + 2) == '.' &&
                            *(uri_read + 3) == '/';
      if (is_parent_path) {
        // Backtrack uri_write
        uri_read = uri_read + 3;
        uri_write--;
        while (*uri_write != '/') {
          uri_write--;
        }
      }
      *uri_write = *uri_read;
      uri_write++;
      uri_read++;
    }
    *uri_write = 0;
  }

  return uri;
}

/* Remove filename from path, and return the folder
 *
 * @return guaranteed to end in '/', and data is copied so may be modified
 */
const char* folder_path_only(const char* in_path) {
  char* uri        = make_uri(in_path);
  char* last_slash = strrchr(uri, '/');
  if (last_slash) {
    *(last_slash + 1) = 0;
    return uri;
  } else {
    return "./";
  }
}

/* This function isn't yet super efficient
 *
 * @param in_base_folder is assumed to be an absolute folder path
 */
char* make_path_relative(const char* in_base_folder, const char* in_change_path) {
  // Create copies of the paths so they can be modified
  char* base_folder = make_uri(in_base_folder);
  char* change_path = make_uri(in_change_path);

  char* output = NULL;

  bool is_absolute_path = (change_path[0] == '/') || (change_path[1] == ':');
  if (!is_absolute_path) {
    output = change_path;
  } else {
    // Find common root
    char* end_common_root = base_folder;
    while ((*end_common_root == *change_path) && *end_common_root && *change_path) {
      end_common_root++;
      change_path++;
    }

    // Count remaining / in end_common_root to determine levels to go up from base
    unsigned num_folders = *end_common_root ? 1 : 0;
    while (*end_common_root) {
      if (*end_common_root == '/') {
        num_folders++;
      }
      end_common_root++;
    }
    bool should_not_count_trailing_slash = num_folders > 0 && *(end_common_root - 1) == '/';
    if (should_not_count_trailing_slash) {
      num_folders--;
    }

    char* parent_folders = (char*)cc_alloc_(3 * num_folders + 1);
    for (unsigned i = 0; i < num_folders; i++) {
      parent_folders[3 * i + 0] = '.';
      parent_folders[3 * i + 1] = '.';
      parent_folders[3 * i + 2] = '/';
    }
    parent_folders[3 * num_folders] = 0;
    output                          = (char*)cc_printf("%s%s", parent_folders, change_path);
  }

  return output;
}
