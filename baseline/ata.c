#include "ata.h"
#include "pci.h"
#include "x86arch.h"
#include "support.h"

#define _ata_debug_
#ifdef  _ata_debug_
#include "c_io.h"
#endif

int _ata_modinit() {
    // Slot:        00:1f.1

    // Vendor:      8086        Intel Corporation
    // Device:      27df        82801G (ICH7 Family) IDE Controller
    // Class:       01          Mass storage controller (IDE Interface)
    // Subclass:      01        IDE controller (IDE Interface)

    // Subvendor:   8086        Intel Corporation
    // Subdevice:   d620        Device

    pcidev *ide = find_dev(0x8086, 0x27df, 0x01, 0x01);
    if (ide == (void*) 0) {
        return -1;
    }

#ifdef _ata_debug_
    c_printf("[ata.c][_ata_modinit]: IDE Contrller @= %x, irq: %x, Bar0: %x, Bar1: %x, Bar2: %x, Bar3: %x, Bar4 %x\n", ide->address, ide->irq, ide->bar0, ide->bar1, ide->bar2, ide->bar3, ide->bar4);
#endif

    //uint16_t command = pci_read_command(*ide);
    //// set to 1 to enable busmaster
    //command |= 0x4;
    //// clear bit 10 to make sure that interrupts are enabled
    //command &= 0xfdff;

    //pci_write_command(*ide, command);

    return 0;
}
