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


void _pci_modinit(){
    pci_dev_count = 0;
    for ( uint16_t b = 0; b <= PCI_MAX_BUS; ++b ) {
        for (uint8_t d = 0; d < PCI_MAX_DEV; ++d ) {
            uint16_t vendor = _pci_read_vendorid(b, d, 0);
            if (vendor != 0xFFFF) {
                for (uint8_t f = 0; f < PCI_MAX_FUNC; ++f ) {
                    pci_devs[pci_dev_count].bus = b;
                    pci_devs[pci_dev_count].device = d;
                    pci_devs[pci_dev_count].func = f;
                    pci_devs[pci_dev_count].address = pci_calc_address(b,d,f,0);
                    pci_devs[pci_dev_count].vendorid = vendor;
                    pci_devs[pci_dev_count].deviceid = _pci_read_deviceid(b, d, f);
                    if ( pci_devs[pci_dev_count].deviceid == 0xFFFF ) continue;
                    pci_devs[pci_dev_count].headertype = _pci_read_headertype(b, d, f);
                    pci_devs[pci_dev_count].classid = _pci_read_classid(b, d, f);
                    pci_devs[pci_dev_count].subclassid = _pci_read_subclassid(b, d, f);
#                   ifdef _pci_debug_
                    c_printf("[pci.c][_pci_modinit]: Device Found. VendorID: %x, DeviceID: %x, ClassID: %x\n", 
                            pci_devs[pci_dev_count].vendorid, pci_devs[pci_dev_count].deviceid, 
                            pci_devs[pci_dev_count].classid);
#                   endif
                    __delay(10);
                    pci_dev_count++;
                }
            }
        }
    }
}


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
    for ( uint8_t d = 0; d < PCI_MAX_DEV; ++d ) {
        // check if vendor is what we are looking for
        if ( vendor != _pci_read_vendorid(bus, d, 0) ) continue;

        // Iterate over functions since vendor is valid
        for ( uint8_t f = 0; f < PCI_MAX_FUNC; ++f ) {

#           ifdef _pci_debug_ //print device results
            uint16_t pci_dev = _pci_read_deviceid(bus, d, f);
            if (pci_dev != 0xFFFF) {
                c_printf("[pci.c][find_dev_bus]: Device Read: %x looking for: %x\n",pci_dev, device);
                __delay(10);
            }
#           endif
            // check device
            if ( device != pci_dev ) continue;

#           ifdef _pci_debug_ //print device results
            uint8_t pci_class = _pci_read_classid(bus, d, f);
            c_printf("[pci.c][find_dev_bus]: Class Read: %x looking for: %x\n",pci_class, class);
            __delay(10);
#           endif
            // check class
            if ( class != pci_class ) continue;

#           ifdef _pci_debug_ //print device results
            uint8_t pci_subclass = _pci_read_subclassid(bus, d, f);
            c_printf("[pci.c][find_dev_bus]: Subclass Read: %x looking for: %x\n",pci_subclass, subclass);
            __delay(10);
#           endif
            // check subclass
            if ( subclass != pci_subclass ) continue;
                   
            return pci_calc_address(bus,d,f,0);
            //@TODO Create pci object to return instead of address.
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
    for ( uint16_t b = 0; b <= PCI_MAX_BUS && result == 0; ++b ) {
        result = find_dev_bus(b,vendor,device,class,subclass);
    }
    return ( result );
}


uint16_t _pci_read_vendorid(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_VENDOR_ID) );
}

uint16_t pci_read_vendorid(pcidev device) {
    return ( pci_read_w(device.bus, device.device, device.func, PCI_VENDOR_ID) );
}

uint16_t _pci_read_deviceid(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) );
}

uint16_t pci_read_deviceid(pcidev device) {
    return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) );
}

uint16_t _pci_read_command(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) );
}

uint16_t pci_read_command(pcidev device) {
    return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) );
}

uint16_t _pci_read_status(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) );
}

uint16_t pci_read_status(pcidev device) {
    return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) );
}

uint8_t _pci_read_classid(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_CLASS) );
}

uint8_t pci_read_classid(pcidev device) {
    return ( pci_read_b(device.bus, device.device, device.func, PCI_CLASS) );
}

uint8_t _pci_read_subclassid(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_SUBCLASS) );
}

uint8_t pci_read_subclassid(pcidev device) {
    return ( pci_read_b(device.bus, device.device, device.func, PCI_SUBCLASS) );
}

uint8_t _pci_read_headertype(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_HEADERTYPE) );
}

uint8_t pci_read_headertype(pcidev device) {
    return ( pci_read_b(device.bus, device.device, device.func, PCI_HEADERTYPE) );
}

uint8_t _pci_read_irq(uint8_t bus, uint8_t dev, uint8_t func) {
    return ( pci_read_b(bus, dev, func, PCI_IRQLINE) );
}

uint8_t pci_read_irq(pcidev device) {
    return ( pci_read_b(device.bus, device.device, device.func, PCI_IRQLINE) );
}

uint8_t pci_read_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    //uint32_t address;
    //uint32_t bus_32  = (uint32_t)bus;
    //uint32_t dev_32 = (uint32_t)dev;
    //uint32_t func_32 = (uint32_t)func;
    //uint32_t base = 0x80000000;
    //address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
    //        (offset & 0xfc) | base);
    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    int8_t res = (uint8_t)((__inl(0xCFC) >> ((offset % 0x04) * 8)) & 0xff);
    return res;
}

uint16_t pci_read_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    //uint32_t address;
    //uint32_t bus_32  = (uint32_t)bus;
    //uint32_t dev_32 = (uint32_t)dev;
    //uint32_t func_32 = (uint32_t)func;
    //uint32_t base = 0x80000000;
    //address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
    //        (offset & 0xfc) | base);
    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    int16_t res = (uint16_t)((__inl(0xCFC) >> ((offset % 0x04) * 8)) & 0xffff);
    return res;
}

uint32_t pci_read_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    //uint32_t address;
    //uint32_t bus_32  = (uint32_t)bus;
    //uint32_t dev_32 = (uint32_t)dev;
    //uint32_t func_32 = (uint32_t)func;
    //uint32_t base = 0x80000000;
    //address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
    //        (offset & 0xfc) | base);
    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    //uint32_t res = (uint32_t)(__inw(0xCFC) & 0xffffffff);
    int32_t res = (uint32_t)(__inl(0xCFC));
    return res;
}

uint32_t pci_calc_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    return address;
}
