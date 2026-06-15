#include <string.h>

#include "../helpers/assert.h"
#include "../helpers/common.c"

void add_vec_c(
    uint64_t * const restrict res,
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t const count
)
{
    uint128_t carry = 0;
    for (uint64_t i = 0; i < count; i++) {
        carry += (uint128_t)n1[i] + n2[i];
        res[i] = LOW(carry);
        carry = HIGH(carry);
    }
}

void add_vec_asm(
    uint64_t * const restrict res,
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t const count
)
{
    memset(res, 0, 2 * count * sizeof(uint64_t));
    __asm__ __volatile__ (

    )
}

void run_add_vec()
{
    constexpr uint64_t count = 100;

    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++) {
        uint64_t n1[count];
        uint64_t n2[count];
        num_rand(n1, count);
        num_rand(n2, count);

        uint64_t res_1[count + 1];
        uint64_t res_2[count + 1];

        add_vec_c(res_1, n1, n2, count);
        add_vec_asm(res_2, n1, n2, count);

        assert(num_eq(res_1, res_2, count + 1));
    }
}