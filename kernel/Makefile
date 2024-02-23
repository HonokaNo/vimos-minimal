include ../env.mk

CFLAGS = $(DEFAULT_COMPILE_OPT) -I../include -I../include/stdlib -nostdinc --target=x86_64-elf -mno-red-zone -ffreestanding -mcmodel=large
ASFLAGS = $(DEFAULT_COMPILE_OPT) -I../include $(DEFAULT_COMPILE_OPT) -O0 --target=x86_64-elf
LDFLAGS = --entry _vMain -z norelro --image-base 0xffff800000000000 --static -Map kernel.map

SRCS := $(wildcard *.S)
SRCS += $(wildcard *.c)
KERNEL_OBJ := $(SRCS:.c=.o)
KERNEL_OBJ := $(KERNEL_OBJ:.S=.o)

.PHONY: kernel
kernel.elf: asm.o $(KERNEL_OBJ) ../stdlib/stdlib.a
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJ) ../stdlib/stdlib.a

.PHONY: clean
clean:
	$(REMOVE) asm.o $(KERNEL_OBJ)
	$(REMOVE) kernel.elf
	$(REMOVE) kernel.map
	$(REMOVE) kernel.elf.*
	$(REMOVE) .depend

depend: $(SRCS)
	$(CC) -I../include -I../include/stdlib -M $(SRCS) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
