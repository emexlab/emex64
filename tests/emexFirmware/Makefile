# Makefile
ASM = emex64asm
LD = emex64ld
VM = emex64vm

# firmware headers
INCDIR = $(abspath ./inc)

ASMFLAGS = -fno_page_align -I$(INCDIR) -DPUT_DRAW_LOGO="1"
LDSCRIPT = $(abspath ./emex64.e64ld)

SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/emex64fw

SRCS = $(wildcard $(SRCDIR)/*.e64)
OBJS = $(patsubst $(SRCDIR)/%.e64,$(BUILDDIR)/%.o,$(SRCS))

LINK_ORDER = \
    $(BUILDDIR)/start.o    \
    $(BUILDDIR)/exc.o      \
    $(BUILDDIR)/interrupt.o \
    $(BUILDDIR)/uart.o     \
    $(BUILDDIR)/put.o      \
    $(BUILDDIR)/mc.o       \
    $(BUILDDIR)/mem.o      \
    $(BUILDDIR)/pmm.o      \
    $(BUILDDIR)/slab.o     \
    $(BUILDDIR)/rtc.o      \
    $(BUILDDIR)/gui.o      \
    $(BUILDDIR)/font.o     \
    $(BUILDDIR)/string.o   \
    $(BUILDDIR)/ac97.o

all: compile execute

compile: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.e64 | $(BUILDDIR)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(TARGET): $(LINK_ORDER) $(LDSCRIPT)
	$(LD) -T $(LDSCRIPT) -o $@ $(LINK_ORDER)

execute:
	$(VM) --firmware $(TARGET) --memory 1 --display on --display:resolution 640 480

clean:
	rm -rf $(BUILDDIR)
