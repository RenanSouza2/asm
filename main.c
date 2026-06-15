#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "exercises/a_add.c"
#include "exercises/b_add_vec.c"
#include "exercises/c_mul.c"
#include "exercises/d_mul_uint.c"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    run_add();
    run_add_vec();
    run_mul();
    run_mul_uint();

    printf("\n");
    return 0;
}
