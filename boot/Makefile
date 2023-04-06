include ../env.mk

BOOT_SRC := boot.c lib.c platform.c
BOOT_HEADER = boot.h lib.h platform.h ../include/uefi.h

boot.efi: $(BOOT_HEADER) $(BOOT_SRC)
	$(COMPILER) $(DEFAULT_COMPILE_OPT) $(BOOT_OPT) -o $@ $(BOOT_SRC)

.PHONY: clean
clean:
	$(REMOVE) boot.efi
