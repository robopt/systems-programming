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

unsigned char ide_read(unsigned char channel, unsigned char reg) {
    unsigned char result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = inb(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        result = inb(channels[channel].ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        result = inb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        outb(channels[channel].base  + reg - 0x00, data);
    else if (reg < 0x0C)
        outb(channels[channel].base  + reg - 0x06, data);
    else if (reg < 0x0E)
        outb(channels[channel].ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_CONTROL, channels[channel].nIEN);
}

void ide_initialize(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) {
    int j, k, count = 0;


