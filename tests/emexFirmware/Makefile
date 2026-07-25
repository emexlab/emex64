# Makefile
VM_DISPLAY ?= on
VM_MEMORY ?= 48

all: FILES ?= src/exc.e64 src/gui.e64 src/interrupt.e64 src/mc.e64 src/put.e64 src/slab.e64 src/string.e64 src/font.e64 src/hang.e64 src/pmm.e64 src/rtc.e64 src/start.e64 src/uart.e64 src/keyboard.e64
all: FLAGS ?= -DPUT_DRAW_LOGO -DFW_LOCK_KTRR -Wl,--nmagic,-v
all: compile execute clean

# Tradeoff is that KTRR can't work in minimal mode (because .text and .data have to both be aligned)
all-minimal: FILES ?= src/exc.e64 src/gui.e64 src/interrupt.e64 src/mc.e64 src/put.e64 src/string.e64 src/font.e64 src/hang.e64 src/pmm.e64 src/rtc.e64 src/start.e64 src/uart.e64 src/keyboard.e64
all-minimal: FLAGS ?= -Wl,--omagic,-v -DFW_MINIMAL
all-minimal: compile execute clean

compile:
	emex64asm $(FILES) -o firmware.img -Iinc -Wl,-T,emex64.e64ld -DUSES_FW_LDSCRIPT -DUSES_FW_CODE $(FLAGS)

execute:
	emex64vm --display $(VM_DISPLAY) -f firmware.img -m:kb $(VM_MEMORY)

clean:
	rm firmware.img
