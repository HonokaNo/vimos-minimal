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

FSDIR = qemu$(PATHSEP)EFI$(PATHSEP)BOOT
MAKE = make
CC = clang
AS = clang
LD = ld.lld
AR = ar
QEMU = qemu-system-x86_64

ifeq ($(DEBUG), 1)
DEBUG_OPT := -O0 -ggdb
QEMU_DEBUG_OPT := -d int -D debug
else
DEBUG_OPT := -O2
endif

DEFAULT_COMPILE_OPT := $(DEBUG_OPT) -Wall -Wextra -mabi=sysv -nostdlib -mno-sse -fno-builtin -fshort-wchar

#QEMU_OPT := -m 1G -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial file:serial -monitor stdio -vga std -netdev user,id=net0,hostfwd=tcp:127.0.0.1:1235-:80 -object filter-dump,id=fiter0,netdev=net0,file=dump.pcap -device e1000,netdev=net0 -device qemu-xhci -device usb-kbd $(QEMU_DEBUG_OPT)
QEMU_OPT := -m 1G -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial file:serial -monitor stdio -vga std -netdev user,id=net0,hostfwd=tcp:127.0.0.1:1235-:80 -object filter-dump,id=fiter0,netdev=net0,file=dump.pcap -device e1000,netdev=net0 -device qemu-xhci -device usb-kbd \
  -d trace:usb_xhci_exit,trace:usb_xhci_run,trace:usb_xhci_stop,trace:usb_xhci_cap_read,trace:usb_xhci_oper_read,trace:usb_xhci_doorbell_read,trace:usb_xhci_oper_write,trace:usb_xhci_port_write,trace:usb_xhci_runtime_write,trace:usb_xhci_doorbell_write,trace:usb_xhci_queue_event,trace:usb_xhci_fetch_trb,trace:usb_xhci_port_reset,trace:usb_xhci_port_link,trace:usb_xhci_port_notify,trace:usb_xhci_slot_enable,trace:usb_xhci_slot_disable,trace:usb_xhci_slot_address,trace:usb_xhci_slot_configure,trace:usb_xhci_slot_evaluate,trace:usb_xhci_slot_reset,trace:usb_xhci_ep_enable,trace:usb_xhci_ep_disable,trace:usb_xhci_ep_set_dequeue,trace:usb_xhci_ep_kick,trace:usb_xhci_ep_stop,trace:usb_xhci_xfer_start,trace:usb_xhci_xfer_async,trace:usb_xhci_xfer_nak,trace:usb_xhci_xfer_retry,trace:usb_xhci_xfer_success,trace:usb_xhci_xfer_error,trace:usb_xhci_unimplemented,trace:usb_xhci_enforced_limit -D debug_xhci \
  $(QEMU_DEBUG_OPT)
#QEMU_OPT := -m 1G -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial file:serial -monitor stdio -vga std -netdev user,id=net0,hostfwd=tcp:127.0.0.1:1234-:80 -object filter-dump,id=fiter0,netdev=net0,file=dump.pcap -device e1000,netdev=net0
#QEMU_OPT := -m 1G -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial tcp:localhost:1234,server,nowait -monitor stdio -vga std -d int -D debug
