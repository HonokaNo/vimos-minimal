include env.mk

default: build disk

.PHONY: init
init:
ifeq ($(OS), Windows_NT)
	where /Q qemu-system-x86_64 & if errorlevel 1 @echo Install Qemu and set the environment variables. && pause && exit

	if not exist OVMF.fd (curl -o OVMF.zip https://jaist.dl.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip --insecure && call powershell -command "Expand-Archive OVMF.zip" && copy %CD%\OVMF\OVMF.fd %CD%\ /V && rd /s /q OVMF && del OVMF.zip)
else
	if ! which ld.lld; then if which apt; then sudo apt install lld -y; fi; fi
	if ! which make; then if which apt; then sudo apt install make -y; fi; fi
	if ! which clang; then if which apt; then sudo apt install clang -y; fi; fi
	if ! which python3; then if which apt; then sudo apt install python3 -y; fi; fi
	if ! which qemu-system-x86_64; then if which apt; then sudo apt install qemu-system-x86 -y; fi; fi
	if ! (echo -n "Binutils: "; ld --version | head -n1 | cut -d" " -f3-); then if which apt; then sudo apt install binutils; fi; fi
endif

.PHONY: init_dir
init_dir:
ifeq ($(OS), Windows_NT)
	if not exist $(FSDIR) mkdir $(FSDIR)
	if not exist qemu$(PATHSEP)dev mkdir qemu$(PATHSEP)dev
else
	mkdir -p $(FSDIR)
	mkdir -p qemu$(PATHSEP)dev
endif

.PHONY: build
build:
	$(MAKE) -C stdlib
	$(MAKE) -C boot
	$(MAKE) -C kernel
	$(MAKE) -C kernel kernel

.PHONY: disk
disk: build
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
	sleep 2
	sudo umount ./mnt
	rmdir mnt
endif

.PHONY: copy_file
copy_file: init_dir build
	$(COPY) boot$(PATHSEP)boot.efi qemu$(PATHSEP)EFI$(PATHSEP)BOOT$(PATHSEP)BOOTX64.EFI
	$(COPY) kernel$(PATHSEP)kernel.elf qemu$(PATHSEP)kernel.elf

.PHONY: mkiso
mkiso: copy_file
	xorriso -as mkisofs -R -f -no-emul-boot -o cdimage.iso qemu

.PHONY: run
run: copy_file
	$(QEMU) $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=fat:rw:qemu

.PHONY: mkfs_run
mkfs_run: disk
	$(QEMU) $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=disk.img

.PHONY: run_dbg
run_dbg: copy_file
	$(QEMU) -s -S $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=fat:rw:qemu

.PHONY: mkfs_run_dbg
mkfs_run_dbg: disk
	$(QEMU) -s -S $(QEMU_OPT) -drive id=ide,index=0,media=disk,format=raw,file=disk.img

.PHONY: clean
clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C stdlib clean
	$(REMOVE) qemu$(PATHSEP)kernel.elf
	$(REMOVE) disk.img
	$(REMOVE) cdimage.iso
