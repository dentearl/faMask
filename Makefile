cflags= -O3 -g -Wall -std=c99 -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Werror --pedantic -funroll-loops -lm 
.PHONY: all clean
all: ./bin/faMask

%.o: %.c %.h
	gcc ${cflags} -c $< -o $@.tmp
	mv $@.tmp $@

bin/faMask: src/faMask.c src/faMask.h src/common.o src/de_hashlib.o
	mkdir -p $(dir $@)
	gcc ${cflags} $^ -o $@.tmp
	mv $@.tmp $@

clean:
	rm -rf ./bin/
