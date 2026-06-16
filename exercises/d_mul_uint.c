#include "../helpers/common.c"

void mul_uint_c(
    uint64_t * const restrict res,
    uint64_t const * const restrict n,
    uint64_t const count,
    uint64_t const value
)
{
    uint128_t carry = 0;
    for (uint64_t i = 0; i < count; i++)
    {
        carry += MUL(n[i], value);
        res[i] = LOW(carry);
        carry = HIGH(carry);
    }
    res[count] = carry;
}

void mul_uint_asm(
    uint64_t * const restrict res,
    uint64_t const * const restrict n,
    uint64_t count,
    uint64_t const value
)
{
    uint64_t carry = 0;
    uint64_t i = 0;
    __asm__ __volatile__ (
        ".intel_syntax noprefix                 \n\t"

        "clc                                    \n\t"

        "loop_begin%=:                          \n\t"

        "mov rax, [%[n] + %[i] * 8]             \n\t"   // A = n[i]
        "mul %[value]                           \n\t"   // (D, A) = MUL(A, value)
        "add rax, %[carry]                      \n\t"   // A += carry
        "mov [%[res] + %[i] * 8], rax           \n\t"   // res[i] = A

        "mov %[carry], rdx                      \n\t"   // carry = D
        "adc %[carry], 0                        \n\t"   // carry += cr

        "lea %[i], [%[i] + 1]                   \n\t"
        "dec %[count]                           \n\t"
        "jnz loop_begin%=                       \n\t"

        "mov [%[res] + %[i] * 8], %[carry]      \n\t"

        ".att_syntax prefix \n\t"
        // out
        :   [carry] "+r" (carry),
            [i] "+r" (i),
            [count] "+r" (count)
        // in
        :   [res] "r" (res),
            [n] "r" (n),
            [value] "r" (value)
        // clobber
        : "cc", "memory", "rax", "rdx"
    );
}

void run_mul_uint()
{
    printf("\nrunning\t %-20s", __func__);

    constexpr uint64_t count = 100;
    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++)
    {
        uint64_t value = rand_64();
        uint64_t n[count];
        num_rand(n, count);

        uint64_t res_2[count + 1];
        uint64_t res_1[count + 1];

        mul_uint_c(res_1, n, count, value);
        mul_uint_asm(res_2, n, count, value);

        assert_num_eq(res_1, res_2, count + 1);
    }

    printf("success");
}