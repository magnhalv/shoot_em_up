#include "logger.h"
#include <cstdlib>

char global_crash_message[512] = {};
bool global_has_crashed = false;

jmp_buf* crash_jump = nullptr;
// This is primarily useful for testing when we want to do
// something else than exiting.
auto set_crash_jump(jmp_buf* jump) -> void {
  crash_jump = jump;
}

void crash_and_burn(const char* msg, ...) {
  va_list args;
  va_start(args, msg);

  va_list args_copy;
  va_copy(args_copy, args);
  log("[CRITICAL ERROR]: ", msg, args_copy);

#if ENGINE_TEST
  int msg_length = vsnprintf(NULL, 0, msg, args_copy);
  vsnprintf(global_crash_message, msg_length + 1, msg, args);
  global_has_crashed = true;
  longjmp(*crash_jump, 1);
#else
  std::exit(1);
#endif
  va_end(args_copy);
  va_end(args);
}
