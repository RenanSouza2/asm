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
    // Working variables
    uint64_t count_dec_1, count_dec_2;
    uint64_t high_1, low_1, high_2, low_2;
    uint64_t carry = 0;
    
    // Working pointers for bumping (to preserve original bases)
    uint64_t const * p_n1;
    uint64_t const * p_n2 = n2;
    uint64_t * p_res;

    __asm__ __volatile__(
        ".intel_syntax noprefix                                 \n\t"

        "xor rax, rax                                           \n\t"
        "clc                                                    \n\t"

        // Setup for First Loop
        "mov rdx, [%[p_n2]]                                     \n\t"  // D = n2[0]
        "mov %[count_dec_1], %[count]                           \n\t"
        "mov %[p_n1], %[n1]                                     \n\t"  // p_n1 = n1
        "mov %[p_res], %[res]                                   \n\t"  // p_res = res

        "first_loop_begin%=:                                    \n\t"
        "mulx %[high_1], %[low_1], [%[p_n1]]                    \n\t"  // (h, l) = MUL(*p_n1, D)
        "adcx %[low_1], %[carry]                                \n\t"  // low_1 += carry + CF
        "mov [%[p_res]], %[low_1]                               \n\t"  // *p_res = low_1
        "mov %[carry], %[high_1]                                \n\t"  // carry = high_1

        "lea %[p_n1], [%[p_n1] + 8]                             \n\t"  // p_n1++
        "lea %[p_res], [%[p_res] + 8]                           \n\t"  // p_res++
        "dec %[count_dec_1]                                     \n\t"
        "jnz first_loop_begin%=                                 \n\t"

        "adcx %[carry], rax                                     \n\t"  // carry += CF (using rax=0)
        "mov [%[p_res]], %[carry]                               \n\t"  // *p_res = carry

        // SECOND LOOP SETUP
        "lea %[p_n2], [%[p_n2] + 8]                             \n\t"  // Advance n2 pointer
        "lea %[count_dec_1], [%[count] - 1]                     \n\t"  // outer counter = count - 1
        "shr %[count], 1                                        \n\t"  // inner counter limit = count / 2

        "second_loop_begin%=:                                   \n\t"

        "clc                                                    \n\t"
        "mov rdx, [%[p_n2]]                                     \n\t"  // D = *p_n2
        "mov %[carry], 0                                        \n\t"
        "mov %[count_dec_2], %[count]                           \n\t"  // Load inner counter
        
        "mov %[p_n1], %[n1]                                     \n\t"  // Reset p_n1 to start of n1
        "lea %[res], [%[res] + 8]                               \n\t"  // Advance outer res base
        "mov %[p_res], %[res]                                   \n\t"  // Reset p_res to new res base

        "second_loop_nested_begin%=:                            \n\t"
        "mulx %[high_1], %[low_1], [%[p_n1]]                    \n\t"  // (h1, l1) = MUL(*p_n1, D)
        "mulx %[high_2], %[low_2], [%[p_n1] + 8]                \n\t"  // (h2, l2) = MUL(*(p_n1+1), D)
        
        "adox %[low_1], [%[p_res]]                              \n\t"  // low_1 += *p_res
        "adcx %[low_1], %[carry]                                \n\t"  // low_1 += carry + CF

        "adox %[low_2], [%[p_res] + 8]                          \n\t"  // low_2 += *(p_res+1) + OF
        "adcx %[low_2], %[high_1]                               \n\t"  // low_2 += high_1 + CF

        "mov [%[p_res]], %[low_1]                               \n\t"  // *p_res = low_1
        "mov [%[p_res] + 8], %[low_2]                           \n\t"  // *(p_res+1) = low_2
        
        "mov %[carry], %[high_2]                                \n\t"  // carry = high_2
        "adox %[carry], rax                                     \n\t"  // carry += OF (using rax=0)

        "lea %[p_n1], [%[p_n1] + 16]                            \n\t"  // p_n1 += 2 elements
        "lea %[p_res], [%[p_res] + 16]                          \n\t"  // p_res += 2 elements
        
        "dec %[count_dec_2]                                     \n\t"
        "jnz second_loop_nested_begin%=                         \n\t"

        "adcx %[carry], rax                                     \n\t"  // carry += CF (using rax=0)
        "mov [%[p_res]], %[carry]                               \n\t"  // Write final carry

        "lea %[p_n2], [%[p_n2] + 8]                             \n\t"  // Advance outer n2 pointer
        "dec %[count_dec_1]                                     \n\t"
        "jnz second_loop_begin%=                                \n\t"

        ".att_syntax prefix                                     \n\t"
        // out
        :   [count_dec_1] "=&r" (count_dec_1),
            [count_dec_2] "=&r" (count_dec_2),
            [high_1] "=&r" (high_1),
            [low_1] "=&r" (low_1),
            [high_2] "=&r" (high_2),
            [low_2] "=&r" (low_2),
            [p_n1] "=&r" (p_n1),
            [p_res] "=&r" (p_res),
            [carry] "+&r" (carry),
            [res] "+&r" (res),
            [count] "+&r" (count),
            [p_n2] "+&r" (p_n2)
        // in
        :   [n1] "r" (n1)
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