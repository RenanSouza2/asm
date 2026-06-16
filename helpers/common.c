#ifndef COMMON_C
#define COMMON_C

#include <stdio.h>
#include <stdlib.h>

#include "../mods/macros/uint.h"
#include "../mods/macros/assert.h"

uint16_t rand_16()
{
    // NOLINTNEXTLINE(cert-msc30-cpp, cert-msc30-c, cert-msc50-cpp)
    return (uint16_t)rand();
}

uint32_t rand_32()
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    return ((uint32_t)rand_16() << 16) | rand_16();
}

uint64_t rand_64()
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    return (U64(rand_32()) << 32) | rand_32();
}

void num_rand(uint64_t * const restrict n, uint64_t const count)
{
    for (uint64_t i = 0; i < count; i++)
    {
        n[i] = rand_64();
    }
}

void assert_num_eq(
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t const count
)
{
    for (uint64_t i = 0; i < count; i++)
    {
        if (n1[i] != n2[i])
        {
            printf("\n");
            printf("\ndiff in index: %lu", i);
            printf("\nn1[%lu]: %lu", i, n1[i]);
            printf("\nn2[%lu]: %lu", i, n2[i]);
            assert(false);
        }
    }
}

#endif