# Makefile
all: compile execute clean

compile:
	emex64asm -o hello.img src/start.e64 -I../emexFirmware/inc -fno-page-align

execute:
	emex64vm --firmware hello.img --display off

clean:
	rm hello.img
