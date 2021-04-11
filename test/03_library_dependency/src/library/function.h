#if defined(_WIN32)
  #if !defined(my_dynamic_library_EXPORTS)
    #define DLL_EXPORT __declspec(dllimport)
  #else
    #define DLL_EXPORT __declspec(dllexport)
  #endif
#elif defined(__clang__)
  #define DLL_EXPORT
#endif

#if defined(__cplusplus)
extern "C" {
#endif

const char* getLibraryString();

DLL_EXPORT const char* getDynamicLibraryString();

#if defined(__cplusplus)
}
#endif
