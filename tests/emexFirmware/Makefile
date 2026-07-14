# Makefile
all: MEMORY ?= 48
all: FLAGS ?= -DPUT_DRAW_LOGO -DFW_LOCK_KTRR
all: compile execute clean

# Tradeoff is that KTRR can't work in minimal mode (because .text and .data have to both be aligned)
all-minimal: FLAGS ?= -Wl,--omagic
all-minimal: MEMORY ?= 40
all-minimal: compile execute clean

compile:
	emex64asm src/*.e64 -o firmware.img -Iinc -Wl,-T,emex64.e64ld -DUSES_FW_LDSCRIPT -DUSES_FW_CODE $(FLAGS)

execute:
	emex64vm --display on -f firmware.img -m:kb $(MEMORY)

clean:
	rm firmware.img
