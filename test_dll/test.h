#ifndef TEST_LIB_H
#define TEST_LIB_H

/* Detect Windows */
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef MYLIB_STATIC
    #define MYLIB_API
  #else
    #ifdef MYLIB_BUILD
      #define MYLIB_API __declspec(dllexport)
    #else
      #define MYLIB_API __declspec(dllimport)
    #endif
  #endif
#else
  /* Linux / macOS */
  #if __GNUC__ >= 4
    #define MYLIB_API __attribute__((visibility("default")))
  #else
    #define MYLIB_API
  #endif
#endif

/* Optional: C++ compatibility */
#ifdef __cplusplus
  #define MYLIB_EXTERN extern "C"
#else
  #define MYLIB_EXTERN extern
#endif

MYLIB_EXTERN MYLIB_API void print_hello();

#endif // TEST_LIB_H