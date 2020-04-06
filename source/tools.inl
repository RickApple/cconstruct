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

const char* strip_path(const char* path) { return strrchr(path, '/') + 1; }

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

const char* cc_printf(const char* format, ...) {
  unsigned length = strlen(format);

  // Guess length of output format
  unsigned alloc_size = (2 * length > 128) ? 2 * length : 128;

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

const char* cc_string_append(const char* a, const char* b ) {
  unsigned length_a = strlen(a);
  unsigned length_b = strlen(b);

  char* out = (char*)cc_alloc_(length_a + length_b + 1);
  memcpy(out, a, length_a);
  memcpy(out+length_a, b, length_b);
  out[length_a+length_b] = 0;
  return out;
}

const char* cc_string_clone(const char* a) {
  unsigned length_a = strlen(a);

  char* out = (char*)cc_alloc_(length_a + 1);
  memcpy(out, a, length_a);
  out[length_a] = 0;
  return out;
}



typedef struct array_header_t {
  unsigned count_;
  unsigned capacity_;
} array_header_t;

void* array_grow( void* a, unsigned element_size);

#define array_header(a) \
    ((a) ? ((array_header_t *)((char *)(a) - sizeof(array_header_t))) : 0)

#define array_count(a) ((a) ? array_header(a)->count_ : 0)

#define array_full(a) ((a) ? (array_header(a)->count_==array_header(a)->capacity_) : true)

#define array_push(a, item) \
    (array_full(a) ? a = (decltype(a)) array_grow(a, sizeof(*a)) : 0, \
    (a[array_header(a)->count_++] = item))

void* array_grow( void* a, unsigned element_size) {
  unsigned prev_capacity = 0;
  unsigned prev_count = 0;
  array_header_t *prev_header = array_header(a);
  if( prev_header ) {
    prev_capacity = prev_header->capacity_;
    prev_count = prev_header->count_;
  }

  unsigned new_capacity = (unsigned)(prev_capacity * 1.75);
  if( new_capacity==prev_capacity ) {
    new_capacity = prev_capacity + 8;
  }

  array_header_t *new_header = (array_header_t*)cc_alloc_(element_size*new_capacity+sizeof(array_header_t));
  void* out = new_header + 1;
  new_header->count_ = prev_count;
  new_header->capacity_ = new_capacity;
  memcpy(out, a, element_size * prev_count);

  // At this point could free the prev data, but not doing so in CConstruct

  return out;
}