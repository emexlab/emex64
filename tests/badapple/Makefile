# Makefile
all: ADDITIONAL_FLAGS := -DFULL=1
all: compile execute clean

all-benchmark: ADDITIONAL_FLAGS := -DBENCHMARK=1
all-benchmark: compile execute clean

all-minimal: ADDITIONAL_FLAGS := -DMINIMAL=1
all-minimal: compile execute clean

compile:
	emex64asm -o badapple.img src/badapple.e64 ../emexFirmware/src/gui.e64 ../emexFirmware/src/uart.e64 ../emexFirmware/src/font.e64 ../emexFirmware/src/interrupt.e64 ../emexFirmware/src/pmm.e64 ../emexFirmware/src/mc.e64 ../emexFirmware/src/string.e64 ../emexFirmware/src/slab.e64 ../emexFirmware/src/put.e64 ../emexFirmware/src/hang.e64 $(ADDITIONAL_FLAGS) -DPUT_DRAW_LOGO=0 -DVFB_ENABLED=0  -I../emexFirmware/inc -Wl,-T,../emexFirmware/emex64.e64ld -DUSES_FW_LDSCRIPT=1 -DUSES_FW_CODE=1

execute:
	emex64vm --display required --display:resolution 256 256 -f badapple.img

clean:
	rm badapple.img
