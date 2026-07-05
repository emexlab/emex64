# Makefile
all: compile execute clean

compile:
	emex64asm src/*.e64 -o firmware.img -Iinc -Wl,-T,emex64.e64ld -DUSES_FW_LDSCRIPT -DUSES_FW_CODE

execute:
	emex64vm --display on -f firmware.img -m:kb 48

clean:
	rm firmware.img
