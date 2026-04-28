.ONESHELL:
.PHONY: fumi

compiler := gcc
cflags := -Wall -Wextra -pedantic -std=c23 -Og -ggdb
clibs := -lmcu-debug

fumi:
	$(compiler) $(cflags) *.c -o ../build/bin/fumi $(clibs)
