# Makefile

VM_DISPLAY ?= on
VM_MEMORY ?= 2
VM_DISKS ?=
VM_USB_DISKS ?=

EMEX64_LLVM ?= ../../llvm/install/bin
CLANG = $(EMEX64_LLVM)/clang
LLD = $(EMEX64_LLVM)/ld.lld
OBJCOPY = $(EMEX64_LLVM)/llvm-objcopy
CFLAGS_FW ?= --target=emex64-unknown-elf -O2 -g0 -Wall -msoft-float -ffreestanding -nostdlibinc -Ic/include -I../../EmexToolchain/include/public

FW_FILES = src/start.e64
FW_CFILES = c/main.c c/string.c c/uart.c c/put.c c/gui.c c/font.c c/logo.c c/interrupt.c c/exc.c c/rtc.c c/mc.c c/keyboard.c c/fwlib.c c/pcie.c c/blk.c c/sata.c c/usb.c c/usbms.c c/boot.c
FW_FLAGS = -DPUT_DRAW_LOGO -Wl,--nmagic,-v

all: FILES ?= $(FW_FILES)
all: CFILES ?= $(FW_CFILES)
all: FLAGS ?= $(FW_FLAGS)
all: compile execute clean

build: FILES ?= $(FW_FILES)
build: CFILES ?= $(FW_CFILES)
build: FLAGS ?= $(FW_FLAGS)
build: compile

comma := ,
ASMFLAGS = $(filter-out -Wl%,$(FLAGS))
CDEFS = $(filter -D%,$(FLAGS))
LDFLAGS = $(subst $(comma), ,$(patsubst -Wl$(comma)%,%,$(filter -Wl%,$(FLAGS))))
COBJS = $(addprefix build/,$(notdir $(CFILES:.c=.o)))

compile:
	mkdir -p build
	emex64asm -r $(FILES) -o $(CURDIR)/build/fw_asm.o $(ASMFLAGS)
	@set -e; for f in $(CFILES); do \
		echo "$(CLANG) $(CFLAGS_FW) $(CDEFS) -c $$f -o build/$$(basename $$f .c).o"; \
		$(CLANG) $(CFLAGS_FW) $(CDEFS) -c $$f -o build/$$(basename $$f .c).o; \
	done;
	$(LLD) -T firmware.ld build/fw_asm.o $(COBJS) $(EMEX64_LLVM)/../lib/emex64/libclang_rt.builtins-softfp.a -o build/firmware.elf
	$(OBJCOPY) -O binary build/firmware.elf firmware.img

execute:
	emex64vm --display $(VM_DISPLAY) --display:resolution 1280 800 -f firmware.img -m:mb $(VM_MEMORY) $(foreach d,$(VM_DISKS),--sata $(d)) $(foreach d,$(VM_USB_DISKS),--usb $(d))

clean:
	rm -rf firmware.img firmware.img.nvram build
