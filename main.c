#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "exercises/a_add.c"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    run_add();

    printf("\n");
    return 0;
}
