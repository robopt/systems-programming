/*
** pci.c
** PCI Bus driver
** Reads from PCI bus
** Enumerates PCI bus looking for devices
** Edward Mead
*/
#include "pci.h"
#include "types.h"
#include "startup.h"
#include "support.h"
#define _pci_debug_
#ifdef _pci_debug_
#include "c_io.h"
#endif

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

    // Iterate over all 32 devices
    for ( int d = 0; d < PCI_MAX_DEV; ++d ) {
        // check if vendor is what we are looking for
        if ( vendor != pci_read_vendor(bus, d, 0) ) continue;

#       ifdef _pci_debug_
        //c_puts("[pci.c][find_dev_bus]: Matching Vendor Found!");
#       endif
        
        // Iterate over functions since vendor is valid
        for ( int f = 0; f < PCI_MAX_FUNC; ++f ) {

#           ifdef _pci_debug_ //print device results
            uint16_t pci_dev = pci_read_device(bus, d, f);
            c_printf("[pci.c][find_dev_bus]: Device Read: %x\n",pci_dev);
            __delay(10);
#           endif
            // check device
            if ( device != pci_dev ) continue;

#           ifdef _pci_debug_ //print device results
            uint8_t pci_class = pci_read_class(bus, d, f);
            c_printf("[pci.c][find_dev_bus]: Class Read: %x\n",pci_dev);
            __delay(10);
#           endif
            // check class
            if ( class != pci_class ) continue;

#           ifdef _pci_debug_ //print device results
            uint8_t pci_subclass = pci_read_subclass(bus, d, f);
            c_printf("[pci.c][find_dev_bus]: Subclass Read: %x\n",pci_dev);
            __delay(10);
#           endif
            // check subclass
            if ( subclass != pci_subclass ) continue;
            

            //@TODO Create pci object to return instead of address.

            /* Calculate address, adapted from http://wiki.osdev.org/PCI */
            uint32_t bus_32  = (uint32_t)bus;
            uint32_t dev_32 = (uint32_t)device;
            uint32_t func_32 = (uint32_t)class;
            uint32_t base = 0x80000000;
            uint8_t offset = 0;
            uint32_t address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
                    (offset & 0xfc) | base);
            return address;
        }
    }
    return 0;
}

/*
** Enumerate the pci bus looking for a certain device
** 
*/
uint32_t find_dev(uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass ) {
    uint32_t result = 0;
    for ( int b = 0; b < PCI_MAX_BUS && result == 0; ++b ) {
        result = find_dev_bus(b,vendor,device,class,subclass);
    }
    return ( result );
}


uint16_t pci_read_vendor(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_VENDOR_ID) );
}

uint16_t pci_read_device(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) );
}

uint8_t pci_read_class(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_CLASS) );
}

uint8_t pci_read_subclass(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_SUBCLASS) );
}

uint8_t pci_read_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    __outl(0xCF8, address);
    uint8_t res = (uint8_t)(__inw(0xCFC) & 0x000000ff);
    return res;
}

uint16_t pci_read_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            offset | base);
    __outl(0xCF8, address);
    //uint8_t off_adj = offset % 0x04;
    //uint16_t res = (uint16_t)((__inw(0xCFC) >> (offset * 8)) & 0xffff);
    //uint16_t res = (uint16_t)(__inw(0xCFC) & 0x0000ffff);
    uint16_t res = (uint16_t)( (__inl(0xCFC) >> (offset % 0x04) ) & 0x0000ffff);
    return res;
}

uint32_t pci_read_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    __outl(0xCF8, address);
    uint32_t res = (uint32_t)(__inw(0xCFC) & 0xffffffff);
    return res;
}
