# Makefile

all: compile execute clean

compile:
	emex64asm -o badapple.o src/*.e64 ../emex64OS/src/mc.e64 ../emex64OS/src/pmm.e64 ../emex64OS/src/interrupt.e64 ../emex64OS/src/mem.e64 -fno_page_align
	cat badapple.o vid.bitmask > badapple
	rm badapple.o

execute:
	emex64vm --bios ./badapple --memory 75

clean:
	rm -rf badapple
