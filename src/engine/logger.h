#pragma once

#include "platform/types.h"
#include <csetjmp>
#include <cstdarg>
#include <cstdio>

extern char global_crash_message[512];
extern bool global_has_crashed;

extern jmp_buf* crash_jump;

inline void log(const char* type, const char* msg, va_list args) {
  printf("%s", type);
  vprintf(msg, args);
  printf("\n");
}

inline void log_info(const char* msg, ...) {
  va_list args;
  va_start(args, msg);

  va_list args_copy;
  va_copy(args_copy, args);
  log("[INFO]: ", msg, args_copy);
  va_end(args_copy);

  va_end(args);
}

inline void log_warning(const char* msg, ...) {
  va_list args;
  va_start(args, msg);

  va_list args_copy;
  va_copy(args_copy, args);
  log("[WARNING]: ", msg, args_copy);
  va_end(args_copy);

  va_end(args);
}

inline void log_error(const char* msg, ...) {
  va_list args;
  va_start(args, msg);

  va_list args_copy;
  va_copy(args_copy, args);
  log("[ERROR]: ", msg, args_copy);
  va_end(args_copy);

  va_end(args);
}

auto set_crash_jump(jmp_buf* exit_jump) -> void;
auto crash_and_burn(const char* msg, ...) -> void;

inline void assert_eq(i32 value1, i32 value2) {
  if (value1 != value2) {
    crash_and_burn("%d is not equal to %d. File: %s:%d", value1, value2, __FILE__, __LINE__);
  }
}

#define ASSERT_LEQ(value1, value2)                                                                           \
  do {                                                                                                       \
    if (value1 > value2) {                                                                                   \
      crash_and_burn("%d is not less than or equal to %d. File: %s:%d", value1, value2, __FILE__, __LINE__); \
    }                                                                                                        \
  } while (0);

#define ASSERT_EQ(value1, value2)                                                               \
  do {                                                                                          \
    if (value1 != value2) {                                                                     \
      crash_and_burn("%d is not equal to %d. File: %s:%d", value1, value2, __FILE__, __LINE__); \
    }                                                                                           \
  } while (0);\
