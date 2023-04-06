include ../env.mk

KERNEL_HEADER := ../include/asm.h ../include/mem.h
KERNEL_OBJ := kernel.o

.PHONY: all
all: $(KERNEL_OBJ)

.PHONY: kernel
kernel:
	$(LINKER) --entry vMain -z norelro --image-base 0x100000 --static -Map kernel.map -o kernel.elf $(KERNEL_OBJ) ../stdlib/stdlib.a

.c.o: $(HEADER)
	$(COMPILER) $(DEFAULT_COMPILE_OPT) $(KERNEL_OPT) -o $@ -c $<
.S.o: $(HEADER)
	$(COMPILER) $(DEFAULT_COMPILE_OPT) -O0 --target=x86_64-elf -o $@ -c $<

.PHONY: clean
clean:
	$(REMOVE) $(KERNEL_OBJ)
	$(REMOVE) kernel.elf
	$(REMOVE) kernel.map
