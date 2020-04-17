#if defined(_MSC_VER)
#include <direct.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define countof(a) sizeof(a) / sizeof(a[0])

// TODO: allow this to be set from the command line
bool is_verbose = false;

#define LOG_ERROR_AND_QUIT(...)   \
  {                               \
    fprintf(stderr, __VA_ARGS__); \
    exit(1);                      \
  }
#define LOG_VERBOSE(...) \
  if (is_verbose) fprintf(stdout, __VA_ARGS__)

const char* strip_path(const char* path) {
  const char* last_slash = strrchr(path, '/');
  if (last_slash)
    return last_slash + 1;
  else
    return path;
}

bool is_header_file(const char* file_path) { return strstr(file_path, ".h") != 0; }

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

#define array_full(a) ((a) ? (array_header(a)->count_ == array_header(a)->capacity_) : true)

#define array_push(a, item)                                                \
  (array_full(a) ? (*((void**)&a) = array_grow((void*)a, sizeof(*a))) : 0, \
   (a[array_header(a)->count_++] = item))

#define array_append(a, data, count)                                       \
  (array_full(a) ? (*((void**)&a) = array_grow((void*)a, sizeof(*a))) : 0, \
   (memcpy(a + array_header(a)->count_, data, (count) * sizeof(*a))),      \
   (array_header(a)->count_ = array_header(a)->count_ + (count)))

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
    assert(false && "Not implemented");
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
    in = out_string;  // Breaking connection to stretchy buffer but that's ok.
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
