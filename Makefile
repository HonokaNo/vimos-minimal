include env.mk

default: disk.img

.PHONY: init
init:
ifeq ($(OS), Windows_NT)
	where /Q qemu-system-x86_64 & if errorlevel 1 @echo Install Qemu and set the environment variables. && pause && exit

	if not exist OVMF.fd (curl -o OVMF.zip https://jaist.dl.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip --insecure && call powershell -command "Expand-Archive OVMF.zip" && copy %CD%\OVMF\OVMF.fd %CD%\ /V && rd /s /q OVMF && del OVMF.zip)
else
	if ! which ld.lld; then if which apt; then sudo apt install lld -y; fi; fi
	if ! which make; then if which apt; then sudo apt install make -y; fi; fi
	if ! which clang; then if which apt; then sudo apt install clang -y; fi; fi
	if ! which qemu-system-x86_64; then if which apt; then sudo apt install qemu-system-x86 -y; fi; fi
	if ! (echo -n "Binutils: "; ld --version | head -n1 | cut -d" " -f3-); then if which apt; then sudo apt install binutils; fi; fi
endif
	$(MAKE) depend

.PHONY: init_dir
init_dir:
ifeq ($(OS), Windows_NT)
	if not exist $(FSDIR) mkdir $(FSDIR)
	if not exist qemu$(PATHSEP)dev mkdir qemu$(PATHSEP)dev
else
	mkdir -p $(FSDIR)
	mkdir -p qemu$(PATHSEP)dev
endif

BOOT_SRCS := $(wildcard boot/*.c)
boot/boot.efi: $(BOOT_SRCS)
	$(MAKE) -C boot

STDLIB_SRCS := $(wildcard stdlib/*.c)
STDLIB_SRCS += $(wildcard stdlib/*.S)
stdlib/stdlib.a: $(STDLIB_SRCS)
	$(MAKE) -C stdlib

kernel/kernel.elf: depend stdlib/stdlib.a
	$(MAKE) -C kernel

disk.img: boot/boot.efi kernel/kernel.elf
ifeq ($(OS), Windows_NT)
	@echo On Windows, img generation will be skipped.
else
	qemu-img create -f raw ./disk.img 200M
	mkfs.fat -F 32 ./disk.img
	mkdir -p ./mnt
	sudo mount -o loop ./disk.img ./mnt
	sudo mkdir -p ./mnt/EFI/BOOT
	sudo cp boot/boot.efi ./mnt/EFI/BOOT/BOOTX64.EFI
	sudo cp kernel/kernel.elf ./mnt/kernel.elf
	sudo cp Makefile ./mnt/Makefile
	sleep 2
	sudo umount ./mnt
	rmdir mnt
endif

# For QEMU, VirtualBox
disk.qcow2: disk.img
	qemu-img convert -O qcow2 disk.img $@

# For VMWare
disk.vmdk: disk.img
	qemu-img convert -O vmdk disk.img $@

# For Hyper-V
disk.vhdx: disk.img
	qemu-img convert -O vhdx disk.img $@

cdimage.iso: copy_file
	xorriso -as mkisofs -R -f -no-emul-boot -o $@ qemu

.PHONY: qcow2
qcow2: disk.qcow2

.PHONY: vmdk
vmdk: disk.vmdk

.PHONY: vhdx
xhdx: disk.vhdx

.PHONY: mkiso
mkiso: cdimage.iso

.PHONY: copy_file
copy_file: boot/boot.efi kernel/kernel.elf
	$(COPY) boot$(PATHSEP)boot.efi $(FSDIR)$(PATHSEP)BOOTX64.EFI
	$(COPY) kernel$(PATHSEP)kernel.elf qemu$(PATHSEP)kernel.elf
	$(COPY) Makefile qemu$(PATHSEP)Makefile

.PHONY: run
run: copy_file
	$(QEMU) $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=fat:rw:qemu,if=ide

.PHONY: mkfs_run
mkfs_run: disk.img
	$(QEMU) $(QEMU_OPT) -drive id=hdd0,media=disk,if=ide,index=0,file=disk.img

.PHONY: qcow2_run
qcow2_run: disk.qcow2
	$(QEMU) $(QEMU_OPT) -drive id=hdd0,media=disk,if=ide,index=0,file=disk.qcow2

.PHONY: run_dbg
run_dbg: copy_file
	$(QEMU) -s -S $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=fat:rw:qemu,if=ide

.PHONY: mkfs_run_dbg
mkfs_run_dbg: disk.img
	$(QEMU) -s -S $(QEMU_OPT) -drive id=hdd0,media=disk,if=ide,index=0,file=disk.img

.PHONY: qcow2_run_dbg
qcow2_run_dbg: disk.qcow2
	$(QEMU) -s -S $(QEMU_OPT) -drive id=hdd0,media=disk,if=ide,index=0,file=disk.qcow2

depend:
	$(MAKE) -C kernel depend

.PHONY: clean
clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C stdlib clean
	$(MAKE) -C app clean
	$(REMOVE) qemu$(PATHSEP)kernel.elf
	$(REMOVE) disk.img disk.qcow2 disk.vmdk disk.vhdx cdimage.iso
