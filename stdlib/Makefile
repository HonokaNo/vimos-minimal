include ../env.mk

CFLAGS = $(DEFAULT_COMPILE_OPT) -I../include/stdlib -nostdinc --target=x86_64-elf -ffreestanding -mcmodel=large
ASFLAGS = $(DEFAULT_COMPILE_OPT) -O0 --target=x86_64-elf

SRCS := $(wildcard *.c)
SRCS += $(wildcard *.S)
SRCS += $(wildcard sys/*.S)
STDLIB_OBJ := $(SRCS:.c=.o)
STDLIB_OBJ := $(STDLIB_OBJ:.S=.o)

stdlib.a: $(STDLIB_OBJ)
	$(AR) r $@ $(STDLIB_OBJ)

.PHONY: clean
clean:
	$(REMOVE) $(STDLIB_OBJ)
	$(REMOVE) stdlib.a
