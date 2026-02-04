#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define DeferLoop_(begin, end, n) for (int CONCAT(_i_, n) = ((begin), 0); !CONCAT(_i_, n); CONCAT(_i_, n) += 1, (end))

#define DeferLoop(begin, end) DeferLoop_(begin, end, __COUNTER__)
