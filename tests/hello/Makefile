# Makefile

all: compile execute clean

compile:
	emex64asm -fno_page_align -o hello src/*.e64

execute:
	emex64vm ./hello

clean:
	rm hello
