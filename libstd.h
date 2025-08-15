#include <cstdint>
#include <cstdio>


int64_t ipow(int64_t a, int64_t n) // NOLINT
{
    if (n == 0) {
        return 1;
    }
    if (n & 1) {
        return a * ipow(a, n - 1);
    }
    double half = ipow(a, n >> 1);
    return half * half;
}

int64_t log2(int64_t x) // NOLINT
{
    int64_t result{};
    for (uint64_t i = 0; i < 64; ++i) {
        if (x & (1 << i))
            result = i;
    }
    return result;
}

int64_t put(int64_t x) // NOLINT
{
    fputc((char)(x + 48), stderr);
    return 0;
}

int64_t print(int64_t x) // NOLINT
{
    fprintf(stderr, "%ld\n", x);
    return 0;
}

int64_t in() // NOLINT
{
    int64_t value;
    scanf("%ld", &value);
    return value;
}
