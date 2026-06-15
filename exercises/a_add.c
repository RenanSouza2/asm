#include "../helpers/assert.h"
#include "../helpers/common.c"

uint64_t add_c(uint64_t const v1, uint64_t const v2)
{
    return v1 + v2;
}

uint64_t add_asm(uint64_t v1, uint64_t const v2)
{
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n\t"

        "add %[v1], %[v2]\n\t"
        
        ".att_syntax prefix \n\t"

        : [v1] "+r" (v1)
        : [v2] "r" (v2)
        : "cc"
    );
    return v1;
}

void run_add()
{
    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++) {
        uint64_t v1 = rand_64();
        uint64_t v2 = rand_64();

        uint64_t res_1 = add_c(v1, v2);
        uint64_t res_2 = add_asm(v1, v2);
        
        assert(res_1 == res_2);
    }

    printf("\nSuccess: %s", __func__);
}