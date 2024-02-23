#ifndef __VIMOS_PCI_H__
#define __VIMOS_PCI_H__

#include <stdint.h>

struct pci_device{
  uint16_t vendor, device;
  uint32_t bar0, bar1, bar2, bar3, bar4, bar5;
};

void init_pci(void);

#endif
