sonLibPath = ../sonLib/lib
cflags= -O0 -g -Wall -std=c99 -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Werror --pedantic -funroll-loops -lm 
.PHONY: all clean
all: ./bin/faMask

%.o: %.c %.h
	gcc ${cflags} -c $< -o $@.tmp
	mv $@.tmp $@

bin/faMask: src/faMask.c src/faMask.h 
	mkdir -p $(dir $@)
	gcc ${cflags} $^ -I ${sonLibPath} ${sonLibPath}/sonLib.a -o $@.tmp
	mv $@.tmp $@

clean:
	rm -rf ./bin/
