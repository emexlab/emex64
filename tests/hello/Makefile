# Makefile
all: compile execute clean

compile:
	emex64asm -o hello.img src/start.e64 -I../emexFirmware/inc -fno-page-align

execute:
	emex64vm -f hello.img -m:kb 16 --display off

clean:
	rm hello.img
