#include "../mods/macros/time.h"

#include "../helpers/common.c"

void mul_vec_c(
    uint64_t * const restrict res,
    uint64_t const * const restrict n2,
    uint64_t const * const restrict n1,
    uint64_t const count
)
{
    uint128_t carry = 0;
    uint64_t value = n2[0];
    for (uint64_t i = 0; i < count; i++)
    {
        carry += MUL(n1[i], value);
        res[i] = LOW(carry);
        carry = HIGH(carry);
    }
    res[count] = carry;

    for (uint64_t i = 1; i < count; i++)
    {
        uint64_t value = n2[i];
        carry = 0;
        for (uint64_t j = 0; j < count; j++)
        {
            carry += res[i + j] + MUL(n1[j], value);
            res[i + j] = LOW(carry);
            carry = HIGH(carry);
        }
        res[i + count] = carry;
    }
}

void mul_vec_asm(
    uint64_t * restrict res,
    uint64_t const * const restrict n1,
    uint64_t const * const restrict n2,
    uint64_t const count
)
{
    uint64_t value, count_dec_1, count_dec_2, j;
    uint64_t i = 0;
    uint64_t carry = 0;
    __asm__ __volatile__(
        ".intel_syntax noprefix \n\t"

        "clc \n\t"

        "mov %[value], [%[n2]] \n\t"
        "mov %[count_dec_1], %[count] \n\t"

        "first_loop_begin%=: \n\t"

        "mov rax, [%[n1] + %[i] * 8] \n\t"  // A = n[i]
        "mul %[value] \n\t"                 // (D, A) = MUL(A, value)
        "add rax, %[carry] \n\t"            // A += carry
        "mov [%[res] + %[i] * 8], rax \n\t" // res[i] = A

        "mov %[carry], rdx \n\t" // carry = D
        "adc %[carry], 0 \n\t"   // carry += cr

        "lea %[i], [%[i] + 1] \n\t"
        "dec %[count_dec_1] \n\t"
        "jnz first_loop_begin%= \n\t"

        "mov [%[res] + %[i] * 8], %[carry] \n\t"

        // SETING UP SECOND LOOP
        "mov %[i], 1 \n\t"                          // i = 1
        "lea %[count_dec_1], [%[count] - 1] \n\t"   // count_dec_1 = count - \n\t"  // count_dec_1 = count - 1
        "second_loop_begin%=: \n\t"

        "mov %[value], [%[n2] + %[i] * 8] \n\t" // value = n2[i]
        "mov %[carry], 0 \n\t"                  // carry = 0

        "mov %[j], 0 \n\t"                      // j = 0
        "mov %[count_dec_2], %[count] \n\t"     // count_dec_2 = count
        "lea %[res], [%[res] + 8] \n\t"         // res += 8
        "second_loop_nested_begin%=: \n\t"

        "mov rax, [%[n1] + %[j] * 8] \n\t"      // A = n1[j]
        "mul %[value] \n\t"                     // (D, A) = MUL(A, value)
        "add rax, %[carry] \n\t"                // A += carry

        "mov %[carry], rdx \n\t"
        "adc %[carry], 0 \n\t"

        "adc [%[res] + %[j] * 8], rax \n\t"
        "adc %[carry], 0 \n\t"

        "lea %[j], [%[j] + 1] \n\t"
        "dec %[count_dec_2] \n\t"
        "jnz second_loop_nested_begin%= \n\t"

        "mov [%[res] + %[j] * 8], %[carry] \n\t"

        "lea %[i], [%[i] + 1] \n\t"
        "dec %[count_dec_1] \n\t"
        "jnz second_loop_begin%= \n\t"

        ".att_syntax prefix \n\t"
        // out
        :   [count_dec_1] "=&r" (count_dec_1),
            [count_dec_2] "=&r" (count_dec_2),
            [value] "=&r" (value),
            [j] "=&r" (j),
            [i] "+r"(i),
            [carry] "+r" (carry),
            [res] "+r"(res)
        // in
        :   [n1] "r"(n1),
            [n2] "r"(n2),
            [count] "r" (count)
        // clobber
        :   "cc",
            "memory",
            "rax",
            "rdx"
    );
}

void run_mul_vec()
{
    printf("\nrunning\t %-20s", __func__);

    TIME_SETUP;

    constexpr uint64_t count = 3000;
    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++)
    {
        uint64_t n1[count], n2[count];
        num_rand(n1, count);
        num_rand(n2, count);
        // for (uint64_t i = 0; i < count; i++)
        // {
        //     n1[i] = i;
        //     n2[i] = i;
        // }


        uint64_t res_2[2 * count];
        uint64_t res_1[2 * count];

        mul_vec_c(res_1, n1, n2, count);
        mul_vec_asm(res_2, n1, n2, count);

        assert_num_eq(res_1, res_2, count + 1);
    }

    TIME_END(t1);

    printf("success\t\t%.3f", dtime(t1));
}