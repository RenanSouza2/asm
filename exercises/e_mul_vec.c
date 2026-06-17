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
    uint64_t * restrict n1,
    uint64_t * restrict n2,
    uint64_t count
)
{
    // Working variables
    uint64_t count_dec_1, count_dec_2;
    uint64_t high, low_1, low_2;
    uint64_t carry = 0, zero = 0;

    __asm__ __volatile__(
        ".intel_syntax noprefix                                 \n\t"

        // Setup for First Loop
        "mov rdx, [%[n2]]                                       \n\t"   // D = n2[0]
        "mov %[count_dec_1], %[count]                           \n\t"
        "mov %[n1], %[n1]                                       \n\t"   // n1 = n1
        "mov %[res], %[res]                                     \n\t"   // res = res

        "first_loop_begin%=:                                    \n\t"
        "mulx %[high], %[low_1], [%[n1]]                        \n\t"   // (high, low_1) = MUL(*n1, D)
        "adcx %[low_1], %[carry]                                \n\t"   // low_1 += carry + CF
        "mov [%[res]], %[low_1]                                 \n\t"   // *res = low_1
        "mov %[carry], %[high]                                  \n\t"   // carry = high

        "lea %[n1], [%[n1] + 8]                                 \n\t"   // n1 += 8
        "lea %[res], [%[res] + 8]                               \n\t"   // res += 8
        "dec %[count_dec_1]                                     \n\t"
        "jnz first_loop_begin%=                                 \n\t"

        "adcx %[carry], %[zero]                                 \n\t"   // carry += CF
        "mov [%[res]], %[carry]                                 \n\t"   // *res = carry

        // SECOND LOOP SETUP

        "lea %[count_dec_1], [%[count] - 1]                     \n\t"   // count_dec_1 = count - 1
        "shr %[count], 1                                        \n\t"   // count /= 2

        "second_loop_begin%=:                                   \n\t"

        "shl %[count], 4                                        \n\t"   // count *= 16
        "sub %[n1], %[count]                                    \n\t"   // n1 -= count
        "sub %[res], %[count]                                   \n\t"   // res -= count 
        "lea %[n2], [%[n2] + 8]                                 \n\t"   // n2 += 8
        "lea %[res], [%[res] + 8]                               \n\t"   // res += 8
        "shr %[count], 4                                        \n\t"   // count /= 16

        "mov rdx, [%[n2]]                                       \n\t"   // D = *n2
        "mov %[carry], 0                                        \n\t"   // carry = 3
        "mov %[count_dec_2], %[count]                           \n\t"   // Load inner counter

        "second_loop_nested_begin%=:                            \n\t"
        "mulx %[high], %[low_1], [%[n1]]                        \n\t"   // (high, low_1) = MUL( *n1   , D)
        "adcx %[low_1], %[carry]                                \n\t"   // low_1 += carry + CF
        "mulx %[carry], %[low_2], [%[n1] + 8]                  \n\t"   // (carry, low_2) = MUL(*(n1+8), D)
        
        "adox %[low_1], [%[res]]                                \n\t"   // low_1 += *res

        "adox %[low_2], [%[res] + 8]                            \n\t"   // low_2 += *(res+8) + OF
        "adcx %[low_2], %[high]                                 \n\t"   // low_2 += high + CF

        "mov [%[res]], %[low_1]                                 \n\t"   // *res = low_1
        "mov [%[res] + 8], %[low_2]                             \n\t"   // *(res+8) = low_2

        "adox %[carry], %[zero]                                 \n\t"   // carry += OF

        "lea %[n1], [%[n1] + 16]                                \n\t"   // n1 += 16
        "lea %[res], [%[res] + 16]                              \n\t"   // res += 16
        
        "dec %[count_dec_2]                                     \n\t"
        "jnz second_loop_nested_begin%=                         \n\t"

        "adcx %[carry], %[zero]                                 \n\t"   // carry += CF
        "mov [%[res]], %[carry]                                 \n\t"   // *res = carry

        "dec %[count_dec_1]                                     \n\t"
        "jnz second_loop_begin%=                                \n\t"

        ".att_syntax prefix                                     \n\t"
        // out
        :   [count_dec_1] "=&r" (count_dec_1),
            [count_dec_2] "=&r" (count_dec_2),
            [high] "=&r" (high),
            [low_1] "=&r" (low_1),
            [low_2] "=&r" (low_2),
            [carry] "+&r" (carry),
            [count] "+&r" (count),
            [n1] "+&r" (n1),
            [n2] "+&r" (n2),
            [res] "+&r" (res)
        // in
        :   [zero] "r" (zero)
        // clobber
        :   "cc",
            "memory",
            "rax",                 // We explicitly clobber rax now
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