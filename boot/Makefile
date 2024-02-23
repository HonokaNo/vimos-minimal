include ../env.mk

CFLAGS = $(DEFAULT_COMPILE_OPT) -I../include -I../include/stdlib -target x86_64-pc-win32-coff -fuse-ld=lld -Wl,"/entry:BootMain" -Wl,"/SUBSYSTEM:efi_application" -mno-stack-arg-probe

BOOT_SRC := $(wildcard *.c)
BOOT_HEADER = $(wildcard *.h) ../include/uefi.h

boot.efi: $(BOOT_HEADER) $(BOOT_SRC)
	$(CC) $(CFLAGS) -o $@ $(BOOT_SRC)

.PHONY: clean
clean:
	$(REMOVE) boot.efi
	$(REMOVE) boot.pdb
