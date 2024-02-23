#include "pci.h"
#include <stdbool.h>
#include "asm.h"
#include "flags.h"
#include "printk.h"
#include "xhci.h"

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

#define PCI_HEADER_VENDOR 0x00
#define PCI_HEADER_DEVICE 0x02
#define PCI_HEADER_COMMAND 0x04
#define PCI_HEADER_STATUS 0x06
#define PCI_HEADER_REVISION 0x08
#define PCI_HEADER_PROG_IF 0x08
#define PCI_HEADER_SUBCLASS 0x0a
#define PCI_HEADER_CLASSCODE 0x0a
#define PCI_HEADER_CACHELINE 0x0c
#define PCI_HEADER_LATENCY 0x0c
#define PCI_HEADER_HEADERTYPE 0x0e
#define PCI_HEADER_BIST 0x0e

#define PCI_HEADER_BAR0 0x10
#define PCI_HEADER_BAR1 0x14
#define PCI_HEADER_BAR2 0x18
#define PCI_HEADER_BAR3 0x1c
#define PCI_HEADER_BAR4 0x20
#define PCI_HEADER_BAR5 0x24

static uint16_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){
  uint32_t addr = (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xfc));

  out32(PCI_CONFIG_ADDRESS, addr);
  return (uint16_t)(in32(PCI_CONFIG_DATA) >> ((offset & 2) * 8) & 0xffff);
}

static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){
  uint32_t addr = (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xfc));

  out32(PCI_CONFIG_ADDRESS, addr);
  return in32(PCI_CONFIG_DATA);
}

static void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data){
  uint32_t addr = (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xfc));

  out32(PCI_CONFIG_ADDRESS, addr);
  out32(PCI_CONFIG_DATA, data);
}

static uint8_t readHeaderType(uint8_t bus, uint8_t dev, uint8_t func){
  return pci_read(bus, dev, func, PCI_HEADER_HEADERTYPE) & 0xff;
}

static bool isSingleFunctionDevice(uint8_t header_type){
  return (header_type & 0x80) == 0;
}

static void scanBus(uint8_t bus);

static void scanFunction(uint8_t bus, uint8_t dev, uint8_t func){
#if TEST_PCI
  uint16_t vendid = pci_read(bus, dev, func, PCI_HEADER_VENDOR);
  uint16_t devid = pci_read(bus, dev, func, PCI_HEADER_DEVICE);

  printk("Found dev %02X.%02X.%02X  %04X %04X\n", bus, dev, func, vendid, devid);
#endif
}

static void scanDevice(uint8_t bus, uint8_t dev){
  scanFunction(bus, dev, 0);

  uint8_t prog_if = (pci_read(bus, dev, 0, PCI_HEADER_PROG_IF) & 0xff00) >> 8;
  uint8_t class_code = (pci_read(bus, dev, 0, PCI_HEADER_CLASSCODE) & 0xff00) >> 8;
  uint8_t subclass_code = (pci_read(bus, dev, 0, PCI_HEADER_SUBCLASS) & 0x00ff) >> 0;
#if 1//TEST_PCI
  printk("pci prog if:%02X\n", prog_if);
  printk("pci class:%02X %02X\n", class_code, subclass_code);
#endif

  uint16_t vendid = pci_read(bus, dev, 0, PCI_HEADER_VENDOR);
  uint16_t devid = pci_read(bus, dev, 0, PCI_HEADER_DEVICE);

  /* Extensible Host Controller(USB3) */
#if 1
  if(class_code == 0x0c && subclass_code == 0x03 && prog_if == 0x30){
#if TEST_PCI
//    printk("command:%02X\n", pci_read(bus, dev, 0, PCI_HEADER_COMMAND));
#endif
    /* bus mastering, memory write, memory space */
    uint32_t data = pci_read32(bus, dev, 0, PCI_HEADER_COMMAND) | 0x04 | 0x10 | 0x02;
    pci_write(bus, dev, 0, PCI_HEADER_COMMAND, data);

#if TEST_PCI
//    printk("command:%02X\n", pci_read(bus, dev, 0, PCI_HEADER_COMMAND));
#endif

    struct pci_device device;
    device.vendor = vendid;
    device.device = devid;
    device.bar0 = pci_read32(bus, dev, 0, PCI_HEADER_BAR0);
    device.bar1 = pci_read32(bus, dev, 0, PCI_HEADER_BAR1);
    device.bar2 = pci_read32(bus, dev, 0, PCI_HEADER_BAR2);
    device.bar3 = pci_read32(bus, dev, 0, PCI_HEADER_BAR3);
    device.bar4 = pci_read32(bus, dev, 0, PCI_HEADER_BAR4);
    device.bar5 = pci_read32(bus, dev, 0, PCI_HEADER_BAR5);

#if TEST_PCI
//    printk("io:%d type:%01X prefetchable:%d\n", device.bar0 & 0x01, device.bar0 & 0x6, device.bar0 & 0x08);
#endif

#if 1
    /* Ehci -> Xhci */
    if(vendid == 0x8086){
      uint32_t superspeed_ports = pci_read32(bus, dev, 0, 0xdc);
      pci_write(bus, dev, 0, 0xd8, superspeed_ports);
      uint32_t ehci2xhci_ports = pci_read32(bus, dev, 0, 0xd4);
      pci_write(bus, dev, 0, 0xd0, ehci2xhci_ports);
    }
#endif

    init_xhci(device);
  }
#endif

  if(isSingleFunctionDevice(readHeaderType(bus, dev, 0))) return;

  for(uint8_t func = 1; func < 8; func++){
    if(pci_read(bus, dev, func, PCI_HEADER_VENDOR) == 0xffff) continue;

    scanFunction(bus, dev, func);
  }
}

static void scanBus(uint8_t bus){
  for(uint8_t dev = 0; dev < 255; dev++){
    if(pci_read(bus, dev, 0, PCI_HEADER_VENDOR) == 0xffff) continue;

    scanDevice(bus, dev);
  }
}

void init_pci(void){
  uint8_t header_type = readHeaderType(0, 0, 0);

  if(isSingleFunctionDevice(header_type)){
    scanBus(0);
  }else{
    for(uint8_t func = 0; func < 8; func++){
      if(pci_read(0, 0, func, PCI_HEADER_VENDOR) == 0xffff) continue;

      scanBus(func);
    }
  }
}
