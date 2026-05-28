# Makefile

all: compile execute clean

compile:
	emex64asm -o emex64OS src/*.e64

execute:
	emex64vm ./emex64OS

clean:
	rm -rf emex64OS
