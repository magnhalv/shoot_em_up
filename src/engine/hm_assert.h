#pragma once 

// TODO: MOVE THIS OUT OF ENGINE
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <sstream>
#include <cstdlib>

#pragma comment(lib, "dbghelp.lib")

// Function to print stack trace
// TODO: Move this to platform layer
inline void print_stack_trace() {
    const int max_frames = 100;
    void* addrlist[max_frames];
    unsigned short frames;
    SYMBOL_INFO* symbol;
    HANDLE process;

    process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    frames = CaptureStackBackTrace(0, max_frames, addrlist, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::ostringstream oss;
    oss << "Stack trace:\n";

    for (unsigned int i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(addrlist[i]), 0, symbol);
        oss << frames - i - 1 << ": " << symbol->Name << " - 0x" << symbol->Address << std::endl;
    }

    std::cerr << oss.str();

    free(symbol);
}

// Define the assert macro for debug builds
#ifdef _DEBUG
    #define HM_ASSERT(expr) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << #expr << ", file " << __FILE__ << ", line " << __LINE__ << std::endl; \
        print_stack_trace(); \
        abort(); \
    }

    #define HM_ASSERT_MSG(expr, msg) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << #expr << ", file " << __FILE__ << ", line " << __LINE__ << std::endl; \
        std::cerr << msg << std::endl; \
        print_stack_trace(); \
        abort(); \
    }
#else
    #define HM_ASSERT(expr) ((void)0)
    #define HM_ASSERT_MSG(expr, msg) ((void)0)
#endif
