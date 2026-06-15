run: main.out
	./main.out

build b: main.out

main.out: main.c
	gcc $^ -o $@ -std=c23 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -D_POSIX_C_SOURCE=200809L -O3

clean c:
	rm -rf main.out