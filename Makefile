build b: main.out

run r: main.out
	./main.out

.PHONY: main.out
main.out: main.c
	$(MAKE) c -s
	gcc $^ -o $@ -std=c23 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -D_POSIX_C_SOURCE=200809L -O3

clean c:
	rm -rf main.out