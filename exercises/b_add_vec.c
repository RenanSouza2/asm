
#include "../helpers/common.c"

void add_vec_c(
    uint64_t * const restrict res,
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t const count
)
{
    uint128_t carry = 0;
    for (uint64_t i = 0; i < count; i++)
    {
        carry += (uint128_t)n1[i] + n2[i];
        res[i] = LOW(carry);
        carry = HIGH(carry);
    }
    res[count] = carry;
}

void add_vec_asm(
    uint64_t * const restrict res,
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t count
)
{
    uint64_t tmp;
    uint64_t i = 0;
    __asm__ __volatile__ (
        ".intel_syntax noprefix \n\t"

        "clc \n\t"

        "loop_begin%=: \n\t"

        "mov %[tmp], [%[n1] + %[i] * 8] \n\t"
        "adc %[tmp], [%[n2] + %[i] * 8] \n\t"
        "mov [%[res] + %[i] * 8], %[tmp] \n\t"

        "lea %[i], [%[i] + 1] \n\t"
        "dec %[count] \n\t"
        "jnz loop_begin%= \n\t"

        "mov %[tmp], 0 \n\t"
        "adc %[tmp], 0 \n\t"
        "mov [%[res] + %[i] * 8], %[tmp] \n\t"
        
        ".att_syntax prefix \n\t"

        // out
        :   [tmp] "=&r" (tmp),
            [i] "+r" (i),
            [count] "+r" (count)
        // in
        :   [res] "r" (res),
            [n1] "r" (n1),
            [n2] "r" (n2)
        // clobber
        : "cc", "memory"
    );
}

void run_add_vec()
{
    printf("\nrunning\t %-20s", __func__);

    constexpr uint64_t count = 100;
    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++)
    {
        uint64_t n1[count];
        uint64_t n2[count];
        num_rand(n1, count);
        num_rand(n2, count);

        uint64_t res_1[count + 1];
        uint64_t res_2[count + 1];

        add_vec_c(res_1, n1, n2, count);
        add_vec_asm(res_2, n1, n2, count);

        assert_num_eq(res_1, res_2, count + 1);
    }
    
    printf("success");
}