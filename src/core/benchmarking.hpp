#if defined(__clang__) || defined(__GNUC__)
#define PretendToWriteF64(Variable) asm volatile("vpxor %0,%0,%0" : "=x"(Variable))
#define ForceCompilerToSetF64(Variable, Value) asm volatile("vmovsd %1,%0" : "=x"(Variable) : "g"(Value))
#define PretendToRead(Variable) asm volatile("" : : "x"(Variable))
#else
#define PretendToWriteF64(Variable) (void)(Variable)
#define ForceCompilerToSetF64(Variable, Value) (Variable) = (Value)
#define PretendToRead(Variable) (void)(Variable)
#pragma message("WARNING: Dead code macros are not available on this compiler.")
#endif
