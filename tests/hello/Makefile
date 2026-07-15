# Makefile
all: compile execute clean

compile:
	emex64asm -o hello.img src/start.e64 -I../emexFirmware/inc -Wl,--omagic,-v

execute:
	emex64vm -f hello.img -m:kb 8 --display off

clean:
	rm hello.img
