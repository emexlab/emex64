# Makefile
all: compile minimum-execute clean

compile:
	emex64asm src/*.e64 -o firmware.img -fno-page-align -Iinc -Wl,-T,emex64.e64ld -DPUT_DRAW_LOGO="1" -DFW_TEST_EXCEPTION="0"

modern-execute:
	emex64vm --display on --display:resolution 1200 800 --firmware firmware.img

minimum-execute:
	emex64vm --display on --display:resolution 512 512 --firmware firmware.img

clean:
	rm firmware.img
