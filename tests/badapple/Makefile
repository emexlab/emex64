# Makefile

all: compile execute clean

compile:
	emex64asm -o badapple.o src/*.e64 ../emex64OS/src/mc.e64 ../emex64OS/src/pmm.e64 ../emex64OS/src/interrupt.e64 -fno_page_align -e_badstart
	cat badapple.o vid.bitmask > badapple
	rm badapple.o

execute:
	emex64vm ./badapple

clean:
	rm -rf badapple
