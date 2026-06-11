# Makefile
ASM = emex64asm
LD = emex64ld
VM = emex64vm

# firmware headers
INCDIR = $(abspath ./../emexFirmware/inc)

ASMFLAGS = -fno_page_align -I$(INCDIR)
LDSCRIPT = $(abspath ./../emexFirmware/emex64.e64ld)

SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/emex64fw

SRCS = $(wildcard $(SRCDIR)/*.e64)
OBJS = $(patsubst $(SRCDIR)/%.e64,$(BUILDDIR)/%.o,$(SRCS))

LINK_ORDER := $(BUILDDIR)/badapple.o

all: all-full

all-full: LINK_ORDER += $(BUILDDIR)/../../emexFirmware/build/put.o \
						$(BUILDDIR)/../../emexFirmware/build/gui.o \
						$(BUILDDIR)/../../emexFirmware/build/uart.o \
						$(BUILDDIR)/../../emexFirmware/build/font.o \
						$(BUILDDIR)/../../emexFirmware/build/interrupt.o \
						$(BUILDDIR)/../../emexFirmware/build/pmm.o \
						$(BUILDDIR)/../../emexFirmware/build/mc.o \
						$(BUILDDIR)/../../emexFirmware/build/mem.o \
						$(BUILDDIR)/../../emexFirmware/build/ac97.o
all-full: ASMFLAGS += -DFULL="1"
all-full: compile execute

all-benchmark: LINK_ORDER += $(BUILDDIR)/../../emexFirmware/build/put.o \
						     $(BUILDDIR)/../../emexFirmware/build/gui.o \
						     $(BUILDDIR)/../../emexFirmware/build/uart.o \
						     $(BUILDDIR)/../../emexFirmware/build/font.o \
						     $(BUILDDIR)/../../emexFirmware/build/interrupt.o \
						     $(BUILDDIR)/../../emexFirmware/build/pmm.o \
						     $(BUILDDIR)/../../emexFirmware/build/mc.o \
						     $(BUILDDIR)/../../emexFirmware/build/mem.o
all-benchmark: ASMFLAGS += -DBENCHMARK="1"
all-benchmark: compile execute

all-minimal: ASMFLAGS += -DMINIMAL="1"
all-minimal: compile execute

compile: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.e64 | $(BUILDDIR)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(TARGET): $(LINK_ORDER) $(LDSCRIPT)
	$(LD) -T $(LDSCRIPT) -o $@ $(LINK_ORDER)

execute:
	$(VM) --firmware $(TARGET) --display required --audio on

clean:
	rm -rf $(BUILDDIR)
