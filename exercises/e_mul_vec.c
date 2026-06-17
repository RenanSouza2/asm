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
    uint64_t count
)
{
    uint64_t count_dec_1, count_dec_2, pos;
    uint64_t high_1, low_1;
    uint64_t high_2, low_2;
    uint64_t carry = 0;
    uint64_t zero = 0;
    __asm__ __volatile__(
        ".intel_syntax noprefix                             \n\t"

        "clc                                                \n\t"

        "mov %[pos], 0                                      \n\t"   // pos = 0
        "mov rdx, [%[n2]]                                   \n\t"   // D = n2[0]
        "mov %[count_dec_1], %[count]                       \n\t"

        "first_loop_begin%=:                                \n\t"
        "mulx %[high_1], %[low_1], [%[n1] + %[pos]]         \n\t"   // (high_1, low_1) = MUL(n1[pos], D)
        "adcx %[low_1], %[carry]                            \n\t"   // low_1 += carry + CF
        "mov [%[res] + %[pos]], %[low_1]                    \n\t"   // res[poa] = low_1
        "mov %[carry], %[high_1]                            \n\t"   // carry = high_1

        "lea %[pos], [%[pos] + 8]                           \n\t"
        "dec %[count_dec_1]                                 \n\t"
        "jnz first_loop_begin%=                             \n\t"

        "adcx %[carry], %[zero]                             \n\t"   // carry += CF
        "mov [%[res] + %[count] * 8], %[carry]              \n\t"   // res[count] = carry

        // SECOND LOOP

        "lea %[count_dec_1], [%[count] - 1]                 \n\t"   // count_dec_1 = count - 1
        "shr %[count], 1                                    \n\t"   // count /= 2
        
        "second_loop_begin%=:                               \n\t"
        
        "clc                                                \n\t"
        "lea %[n2], [%[n2] + 8]                             \n\t"   // n2 += 8
        "mov rdx, [%[n2]]                                   \n\t"   // D = n2
        "mov %[carry], 0                                    \n\t"   // carry = 0

        "mov %[count_dec_2], %[count]                       \n\t"   // count_dec_2 = count / 2
        "mov %[pos], 0                                      \n\t"   // pos = 0
        "lea %[res], [%[res] + 8]                           \n\t"   // res += 8
        
        "second_loop_nested_begin%=:                        \n\t"
        "mulx %[high_1], %[low_1], [%[n1] + %[pos]]         \n\t"   // (high_1, low_1) = MUL(n1[pos], D)
        "mulx %[high_2], %[low_2], [%[n1] + %[pos] + 8]     \n\t"   // (high_2, low_2) = MUL(n1[pos + 1], D)

        "adox %[low_1], [%[res] + %[pos]]                   \n\t"   // low_1 += res
        "adcx %[low_1], %[carry]                            \n\t"   // low_1 += carry + CF
        "mov [%[res] + %[pos]], %[low_1]                    \n\t"   // res = low_1
        
        "adox %[low_2], [%[res] + %[pos] + 8]               \n\t"   // low_2 += res + OF
        "adcx %[low_2], %[high_1]                           \n\t"   // low_2 += high_1 + CF
        "mov [%[res] + %[pos] + 8], %[low_2]                \n\t"   // res = low_2

        "mov %[carry], %[high_2]                            \n\t"   // carry = high_2
        "adox %[carry], %[zero]                             \n\t"   // carry += OF

        "lea %[pos], [%[pos] + 16]                          \n\t"
        "dec %[count_dec_2]                                 \n\t"
        "jnz second_loop_nested_begin%=                     \n\t"

        "adcx %[carry], %[zero]                             \n\t"   // carry += CF
        "mov [%[res] + %[pos]], %[carry]                    \n\t"   // res = carry

        "dec %[count_dec_1]                                 \n\t"
        "jnz second_loop_begin%=                            \n\t"

        ".att_syntax prefix                                 \n\t"
        // out
        :   [count_dec_1] "=&r" (count_dec_1),
            [count_dec_2] "=&r" (count_dec_2),
            [high_1] "=&r" (high_1),
            [low_1] "=&r" (low_1),
            [high_2] "=&r" (high_2),
            [low_2] "=&r" (low_2),
            [pos] "=&r" (pos),
            [carry] "+r" (carry),
            [res] "+r"(res),
            [count] "+r" (count),
            [zero] "+r" (zero)
        // in
        :   [n1] "r"(n1),
            [n2] "r"(n2)
        // clobber
        :   "cc",
            "memory",
            "rdx"
    );
}

void run_mul_vec()
{
    printf("\nrunning\t %-20s", __func__);

    uint64_t total_time = 0;

    constexpr uint64_t count = 3000;
    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++)
    {
        uint64_t n1[count], n2[count];
        num_rand(n1, count);
        num_rand(n2, count);

        uint64_t res_2[2 * count];
        uint64_t res_1[2 * count];

        mul_vec_c(res_1, n1, n2, count);

        TIME_SETUP
        mul_vec_asm(res_2, n1, n2, count);
        TIME_END(t1);

        total_time += t1;

        assert_num_eq(res_1, res_2, 2 * count);
    }

    printf("success\t\t%.3f", dtime(total_time));
}