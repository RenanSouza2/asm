#include "../helpers/common.c"

void mul_c(uint64_t * const restrict res, uint64_t v1, uint64_t v2)
{
    uint128_t u = MUL(v1, v2);
    res[0] = LOW(u);
    res[1] = HIGH(u);
}

void mul_asm(
    uint64_t * const restrict res,
    uint64_t v1,
    uint64_t v2
)
{
    __asm__ __volatile__ (
        ".intel_syntax noprefix \n\t"

        "mov rax, %[v1] \n\t"
        "mul %[v2] \n\t"
        "mov [%[res] + 0 * 8], rax \n\t"
        "mov [%[res] + 1 * 8], rdx \n\t"

        ".att_syntax prefix \n\t"
        // out
        :
        // in
        :   [res] "r" (res),
            [v1] "r" (v1),
            [v2] "r" (v2)
        // clobber
        : "cc", "memory", "rax", "rdx"
    );
}

void run_mul()
{
    printf("\nrunning\t %-20s", __func__);

    constexpr uint64_t runs = 1000;
    for (uint64_t i = 0; i < runs; i++)
    {
        uint64_t v1 = rand_64();
        uint64_t v2 = rand_64();

        uint64_t res_1[2];
        uint64_t res_2[2];

        mul_c(res_1, v1, v2);
        mul_asm(res_2, v1, v2);

        assert_num_eq(res_1, res_2, 2);
    }

    printf("success");
}