FLAGS =  -std=c23 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -D_POSIX_C_SOURCE=200809L -march=native
FALGS_PRD = -O3
FLAGS_DBG = -O0 -g -fsanitize=address

build b: main.out
dbg d: dbg.out

run r: main.out
	./main.out

dbg_run dr: dbg.out
	./dbg.out

.PHONY: main.out
main.out: main.c
	$(MAKE) c -s
	gcc $^ -o $@ $(FLAGS) $(FALGS_PRD)

.PHONY: dbg.out
dbg.out: main.c
	$(MAKE) c -s
	gcc $^ -o $@ $(FLAGS) $(FALGS_DBG)

clean c:
	rm -rf *.out