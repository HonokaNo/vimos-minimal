ifeq ($(OS), Windows_NT)
COPY := copy
REMOVE := del
PATHSEP := \\
OVMF = ./OVMF.fd
else
COPY := cp
REMOVE := rm -f
PATHSEP := /
OVMF = /usr/share/ovmf/OVMF.fd
endif

MAKE = make
FSDIR = qemu$(PATHSEP)EFI$(PATHSEP)BOOT
COMPILER = clang
ASSEMBLER = as
LINKER = ld.lld
AR = ar
QEMU = qemu-system-x86_64

DEFAULT_COMPILE_OPT := -O2 -Wall -Wextra -mabi=sysv -nostdlib -mno-sse

BOOT_OPT := -I../include -I../include/stdlib -target x86_64-pc-win32-coff -fuse-ld=lld -Wl,"/entry:BootMain" -Wl,"/SUBSYSTEM:efi_application"
KERNEL_OPT := -I../include -I../include/stdlib -g -nostdinc --target=x86_64-elf -mno-red-zone -ffreestanding -fshort-wchar -fno-builtin
STDLIB_OPT := -I../include/stdlib -nostdinc --target=x86_64-elf -ffreestanding -mcmodel=large -fshort-wchar -fno-builtin

QEMU_OPT := -m 1G -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial file:serial -monitor stdio
