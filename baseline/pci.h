/*
** pci.h
** PCI Bus driver
** Reads from PCI bus
** Enumerates PCI bus looking for devices
** Edward Mead
*/
#include "types.h"

#ifndef _pci_h
#define _pci_h


#define PCI_MAX_BUS 256
#define PCI_MAX_DEV 32
#define PCI_MAX_FUNC 8
/*
** PCI Config Offsets
** Taken from: https://github.com/scialex/reenix/blob/vfs/kernel/include/drivers/pci.h
*/
#define PCI_VENDOR_ID   0x00
#define PCI_DEVICE_ID   0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION    0x08
#define PCI_PROGIF      0x09
#define PCI_CLASS       0x0B
#define PCI_SUBCLASS    0x0A
#define PCI_INTERFACE   0x09
#define PCI_HEADERTYPE  0x0E
#define PCI_BAR0        0x10
#define PCI_BAR1        0x14
#define PCI_BAR2        0x18
#define PCI_BAR3        0x1C
#define PCI_BAR4        0x20
#define PCI_BAR5        0x24
#define PCI_CAPLIST     0x34
#define PCI_IRQLINE     0x3C

typedef struct pci_dev_s {
    uint32_t address;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint16_t vendorid;
    uint16_t deviceid;
    uint8_t revision;
    uint8_t progif;
    uint8_t bus;
    uint8_t device;
    uint8_t func;
    uint8_t classid;
    uint8_t subclassid;
    uint8_t headertype;
    uint8_t irq;
} pcidev;

pcidev pci_devs[256];
uint8_t pci_dev_count;

/*
** Initialize the PCI bus, discover all devices
*/
void _pci_modinit(void);

uint16_t pci_device_count(void);
/*
** Search for a device on a certain PCI bus
*/
//uint32_t find_dev_bus(uint8_t bus, uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass );

/*
** Enumerate the pci bus looking for a certain device
*/
pcidev *find_dev(uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass );

/*
** Read a Vendor from a certain device
*/
uint32_t _pci_read_bar0(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar0(pcidev device);
uint32_t _pci_read_bar1(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar1(pcidev device);
uint32_t _pci_read_bar2(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar2(pcidev device);
uint32_t _pci_read_bar3(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar3(pcidev device);
uint32_t _pci_read_bar4(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar4(pcidev device);
uint32_t _pci_read_bar5(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_read_bar5(pcidev device);
/*
** Read a Vendor from a certain device
*/
uint16_t _pci_read_vendorid(uint8_t bus, uint8_t dev, uint8_t func);
uint16_t pci_read_vendorid(pcidev device);

/*
** Read a Device ID from a certain device
*/
uint16_t _pci_read_deviceid(uint8_t bus, uint8_t dev, uint8_t func);
uint16_t pci_read_deviceid(pcidev device);

/*
** Read Command Register from a certain device
*/
uint16_t _pci_read_command(uint8_t bus, uint8_t dev, uint8_t func);
uint16_t pci_read_command(pcidev device);

/*
** Write Command Register from a certain device
*/
static void _pci_write_command(uint8_t bus, uint8_t dev, uint8_t func, uint8_t val);
static void pci_write_command(pcidev device, uint8_t val);

/*
** Read Status Register from a certain device
*/
uint16_t _pci_read_status(uint8_t bus, uint8_t dev, uint8_t func);
uint16_t pci_read_status(pcidev device);

/*
** Read a HeaderType from a certain device
*/
uint8_t _pci_read_headertype(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_headertype(pcidev device);

/*
** Read a Class from a certain device
*/
uint8_t _pci_read_progif(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_progif(pcidev device);

/*
** Read a Class from a certain device
*/
uint8_t _pci_read_revision(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_revision(pcidev device);

/*
** Read a Class from a certain device
*/
uint8_t _pci_read_classid(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_classid(pcidev device);

/*
** Read a Subclass from a certain device
*/
uint8_t _pci_read_subclassid(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_subclassid(pcidev device);

/*
** Read IRQ line from a certain device
*/
uint8_t _pci_read_irq(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_read_irq(pcidev device);

/*
** Read a byte from a certain bus, device, function, and offset
*/
uint8_t pci_read_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);

/*
** Read a word from a certain bus, device, function, and offset
*/
uint16_t pci_read_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);

/*
** Read an int from a certain bus, device, function, and offset
*/
uint32_t pci_read_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);

/*
** Write a byte to a certain bus, device, function, and offset
*/
static void pci_write_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t val);

/*
** Write a word to a certain bus, device, function, and offset
*/
static void pci_write_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t val);

/*
** Write a long to a certain bus, device, function, and offset
*/
static void pci_write_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t val);

/*
** Calculate the address for a certain device
*/
uint32_t pci_calc_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);

#endif
