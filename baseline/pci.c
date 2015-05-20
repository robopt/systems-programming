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
** Initialize all devices on pci bus
*/
void _pci_modinit(){
    pci_dev_count = 0;
    for ( uint16_t b = 0; b < PCI_MAX_BUS; ++b ) {
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
                    //pci_devs[pci_dev_count].bar0 = _pci_read_bar0(b, d, f) & 0xFFFFFFFC;
                    pci_devs[pci_dev_count].bar0 = _pci_read_bar0(b, d, f);
                    pci_devs[pci_dev_count].bar1 = _pci_read_bar1(b, d, f);
                    pci_devs[pci_dev_count].bar2 = _pci_read_bar2(b, d, f);
                    pci_devs[pci_dev_count].bar3 = _pci_read_bar3(b, d, f);
                    pci_devs[pci_dev_count].bar4 = _pci_read_bar4(b, d, f);
                    pci_devs[pci_dev_count].bar5 = _pci_read_bar5(b, d, f);
                    pci_devs[pci_dev_count].headertype = _pci_read_headertype(b, d, f);
                    pci_devs[pci_dev_count].classid = _pci_read_classid(b, d, f);
                    pci_devs[pci_dev_count].subclassid = _pci_read_subclassid(b, d, f);
                    pci_devs[pci_dev_count].progif = _pci_read_progif(b, d, f);
                    pci_devs[pci_dev_count].revision = _pci_read_revision(b, d, f);
                    pci_devs[pci_dev_count].irq = _pci_read_irq(b, d, f);
#                   ifdef _pci_debug_
                    c_printf("[pci.c][_pci_modinit]: Device Found. VendorID: %x, DeviceID: %x, ClassID: %x\n", 
                            pci_devs[pci_dev_count].vendorid, pci_devs[pci_dev_count].deviceid, 
                            pci_devs[pci_dev_count].classid);
#                   endif
                    pci_dev_count++;
                }
            }
        }
    }
#   ifdef _pci_debug_
    c_printf("[pci.c][_pci_modinit]: Load Complete. Devices Found: %d\n", pci_dev_count);
#   endif
}


/*
** Search for a certain device. Returns NULL if not found.
*/
pcidev *find_dev(uint16_t vendor, uint16_t device, uint8_t class, uint8_t subclass ) {
    for ( uint16_t i = 0; i < pci_dev_count; ++i ) {
        pcidev dev = pci_devs[i];
        if (dev.vendorid == vendor &&
                dev.deviceid == device &&
                dev.classid == class &&
                dev.subclassid == subclass) {
#           ifdef _pci_debug_ 
            c_printf("[pci.c][find_dev] Device found! Vendor: %x, Device: %x, Class: %x, Subclass: %x\n", vendor, device, class, subclass);
#           endif            
            return &pci_devs[i];
        }
    }
    return ( 0 );
}

uint16_t pci_device_count(){ return pci_dev_count; }



uint32_t _pci_read_bar0(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR0) ); }
uint32_t pci_read_bar0(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR0) ); }

uint32_t _pci_read_bar1(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR1) ); }
uint32_t pci_read_bar1(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR1) ); }

uint32_t _pci_read_bar2(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR2) ); }
uint32_t pci_read_bar2(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR2) ); }

uint32_t _pci_read_bar3(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR3) ); }
uint32_t pci_read_bar3(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR3) ); }

uint32_t _pci_read_bar4(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR4) ); }
uint32_t pci_read_bar4(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR4) ); }

uint32_t _pci_read_bar5(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_l(bus, dev, func, PCI_BAR5) ); }
uint32_t pci_read_bar5(pcidev device) { return ( pci_read_l(device.bus, device.device, device.func, PCI_BAR5) ); }

uint16_t _pci_read_vendorid(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_w(bus, dev, func, PCI_VENDOR_ID) ); }
uint16_t pci_read_vendorid(pcidev device) { return ( pci_read_w(device.bus, device.device, device.func, PCI_VENDOR_ID) ); }

uint16_t _pci_read_deviceid(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) ); }
uint16_t pci_read_deviceid(pcidev device) { return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) ); }

uint16_t _pci_read_command(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) ); }
uint16_t pci_read_command(pcidev device) { return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) ); }

uint16_t _pci_read_status(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_w(bus, dev, func, PCI_DEVICE_ID) ); }
uint16_t pci_read_status(pcidev device) { return ( pci_read_w(device.bus, device.device, device.func, PCI_DEVICE_ID) ); }

uint8_t _pci_read_progif(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_PROGIF) ); }
uint8_t pci_read_progif(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_PROGIF) ); }

uint8_t _pci_read_revision(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_REVISION) ); }
uint8_t pci_read_revision(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_REVISION) ); }

uint8_t _pci_read_classid(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_CLASS) ); }
uint8_t pci_read_classid(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_CLASS) ); }

uint8_t _pci_read_subclassid(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_SUBCLASS) ); }
uint8_t pci_read_subclassid(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_SUBCLASS) ); }

uint8_t _pci_read_headertype(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_HEADERTYPE) ); }
uint8_t pci_read_headertype(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_HEADERTYPE) ); }

uint8_t _pci_read_irq(uint8_t bus, uint8_t dev, uint8_t func) { return ( pci_read_b(bus, dev, func, PCI_IRQLINE) ); }
uint8_t pci_read_irq(pcidev device) { return ( pci_read_b(device.bus, device.device, device.func, PCI_IRQLINE) ); }



/*
** Read byte from pci bus
*/
uint8_t pci_read_b(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    int8_t res = (uint8_t)((__inl(0xCFC) >> ((offset % 0x04) * 8)) & 0xff);
    return res;
}

/*
** Read word from pci bus
*/
uint16_t pci_read_w(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    int16_t res = (uint16_t)((__inl(0xCFC) >> ((offset % 0x04) * 8)) & 0xffff);
    return res;
}

/*
** Read long from pci bus
*/
uint32_t pci_read_l(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {

    __outl(0xCF8, pci_calc_address(bus,dev,func,offset));
    int32_t res = (uint32_t)(__inl(0xCFC));
    return res;
}

/*
** Calculate 32bit address from bus, device, function and offset
*/
uint32_t pci_calc_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){
    /* Calculate address, adapted from http://wiki.osdev.org/PCI */
    uint32_t address;
    uint32_t bus_32  = (uint32_t)bus;
    uint32_t dev_32 = (uint32_t)dev;
    uint32_t func_32 = (uint32_t)func;
    uint32_t base = 0x80000000;
    address = (uint32_t) ( (bus_32<<16) | (dev_32 << 11) | (func_32 << 8) |
            (offset & 0xfc) | base);
    return address;
}
