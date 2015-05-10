/*
** pci.c
** PCI Bus driver
** Reads from PCI bus
** Enumerates PCI bus looking for devices
** Edward Mead
*/


// Vendor: 8086 - Intel Corporation
// Device: 1019 - 82547EI Gigabit Ethernet Controller
// Class: 0x02 - Network Controller
// Subclass: 0x00 - Ethernet Controller


/*
** Search for a device on a certain PCI bus
** Param [ bus ]: Bus to search on
** Param [ vendor ]: Vendor to search for
** Param [ device ]: Device to search for
** Param [ class ]: Class to search for
** Param [ subclass ]: Subclass to search for
** Returns device numb or -1 if not found.
*/
uint32_t find_dev_bus(uint8_t bus, uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass ) {
    for ( int d = 0; d < 32; ++d ) {
        // check vendor, device, class, subclass
        if ( vendor != pci_read_vendor(bus, d) ) continue;
        if ( device != pci_read_device(bus,d) ) continue;
        if ( class != pci_read_class(bus,d) ) continue;
        if ( subclass != pci_read_class(bus,d) ) continue;
        uint32_t address;
        uint32_t bus_32  = (uint32_t)bus;
        uint32_t dev_32 = (uint32_t)device;
        uint32_t func_32 = (uint32_t)class;
        uint32_t base = 0x80000000;
        uint32_t address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
                (offset & 0xfc) | base);
        return address;
    }
    return 0;
}

/*
** Enumerate the pci bus looking for a certain device
** 
*/
uint32_t find_dev(uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass ) {
    uint32_t result = 0;
    for ( int b = 0; b < 256 && result == 0; ++b ) {
        result = find_dev_bus(b,vendor,device,class,subclass);
    }
    return ( result );
}


uint16_t pci_read_vendor(uint8_t bus, uint8_t dev) {
    return ( pci_read_w(bus,dev,0,0) );
}

uint16_t pci_read_device(uint8_t bus, uint8_t dev) {
    return ( pci_read_w(bus,dev,0,2) );
}

uint8_t pci_read_class(uint8_t bus, uint8_t dev) {
    return ( pci_read_b(bus,dev,0,11) );
}

uint8_t pci_read_subclass(uint8_t bus, uint8_t dev) {
    return ( pci_read_b(bus,dev,0,10) );
}


uint8_t pci_read_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    return ( (uint8_t) __inb( address ) );
}

uint16_t pci_read_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    return ( (uint16_t) __inw( address ) );
}
uint32_t pci_read_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    return ( (uint32_t) __inl( address ) );
}
