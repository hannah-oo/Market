CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm
BINARIES=pe_exchange pe_trader buy_and_sell.o read_file_and_pipe.o amend_and_cancel.o orderbook_and_insert_node.o
 
all: $(BINARIES)

.PHONY: clean
clean:
	rm -f $(BINARIES)

tests: 
	echo no tests

run_tests:
	echo no tests
