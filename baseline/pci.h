/*
** pci.h
** PCI Bus driver
** Reads from PCI bus
** Enumerates PCI bus looking for devices
** Edward Mead
*/


/*
** Search for a device on a certain PCI bus
*/
uint32_t find_dev_bus(uint8_t bus, uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass );

/*
** Enumerate the pci bus looking for a certain device
*/
uint32_t find_dev(uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass );

/*
** Read a Vendor from a certain device
*/
uint16_t pci_read_vendor(uint8_t bus, uint8_t dev);

/*
** Read a Device ID from a certain device
*/
uint16_t pci_read_device(uint8_t bus, uint8_t dev);

/*
** Read a Class from a certain device
*/
uint8_t pci_read_class(uint8_t bus, uint8_t dev);

/*
** Read a Subclass from a certain device
*/
uint8_t pci_read_subclass(uint8_t bus, uint8_t dev);


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
