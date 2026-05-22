# Makefile

all: compile execute clean

compile:
	emex64asm -o badapple.o src/*.l64
	cat badapple.o vid.bitmask > badapple
	rm badapple.o

execute:
	emex64vm ./badapple

clean:
	rm -rf badapple
