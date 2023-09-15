# Modified from https://github.com/NJU-ProjectN/am-kernels/blob/master/tests/cpu-tests/Makefile
.PHONY: all run gdb clean $(ALL)

COLOR_RED   = \033[1;31m
COLOR_GREEN = \033[1;32m
COLOR_NONE  = \033[0m

ALL = $(basename $(notdir $(shell find . -name "*.c")))

all: $(addprefix Makefile., $(ALL))
	@echo "" $(ALL)

$(ALL): %: Makefile.%

Makefile.%: %.c
	@/bin/echo -e "NAME = $*\nSRCS = $<\ninclude ../Makefile" > $@
	@if make -s -f $@ $(MAKECMDGOALS); then \
		printf "[%14s] $(COLOR_GREEN)PASS!$(COLOR_NONE)\n" $*; \
	else \
		printf "[%14s] $(COLOR_RED)FAIL!$(COLOR_NONE)\n" $*; \
	fi
	-@rm -f Makefile.$*

run: all

gdb: all

clean:
	rm -rf Makefile.* build/
