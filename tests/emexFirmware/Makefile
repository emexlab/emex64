# Makefile
all: compile execute clean

compile:
	emex64asm src/*.e64 -o firmware.img -fno-page-align -Iinc -Wl,-T,emex64.e64ld -DPUT_DRAW_LOGO="1" -DFW_TEST_EXCEPTION="0" -DUSES_FW_LDSCRIPT="1" -DUSES_FW_CODE="1"

execute:
	emex64vm --display on -f firmware.img -m:kb 48

clean:
	rm firmware.img
