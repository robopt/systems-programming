// help from: http://wiki.osdev.org/PCI_IDE_Controller#Read.2FWrite_From_ATA_Drive
// and https://github.com/agargiulo/DOSS/blob/master/disk/disk.c
// and https://github.com/scialex/reenix/blob/vfs/kernel/drivers/blockdev/disk/ata.rs
// and http://www.ata-atapi.com/atadrvr.html

#include "ata.h"
#include "pci.h"
#include "x86arch.h"
#include "support.h"
#include "ulib.h"
#include "klib.h"

#define _ata_debug_
#ifdef  _ata_debug_
#include "c_io.h"
#endif

/*
**  struct declaration for ATA channel
**  each motherboard can support up to two ATA channels
**
**  contains info on which port is used for commands and control,
**  as well as the bus master IDE
*/
static struct ata_channel {
    uint32_t base;      // base port for cmd registers
    uint32_t ctrl;      // base port for ctrl registers
    uint32_t bmide;     // bus master IDE
    uint8_t  interrupt;
} channels[2];

/*
**  struct declaration for the detected IDE device
**
**  contains in on whether or not drive exists, channel, master/ slave, type,
**  and more
*/
struct ide_device {
    uint8_t reserved;       // 0 (Empty) or 1 (This Drive really exists).
    uint8_t channel;        // 0 (Primary Channel) or 1 (Secondary Channel).
    uint8_t drive;          // 0 (Master Drive) or 1 (Slave Drive).
    uint16_t type;          // 0: ATA, 1:ATAPI.
    uint16_t signature;     // Drive Signature
    uint16_t capabilities;  // Features.
    uint32_t commandSets;   // Command Sets Supported.
    uint32_t size;          // Size in Sectors.
    uint8_t model[41];      // Model in string.
} ide_devices[4];

uint8_t ide_buf[2048] = {0};    // buffer used to read the identification space
                                // and fill the ide_device struct

unsigned char ide_irq_invoked = 0;  // used to disable all channel interrupts
                                    // from connected drives

int _ata_modinit() {
    // Slot:        00:1f.1

    // Vendor:      8086        Intel Corporation
    // Device:      27df        82801G (ICH7 Family) IDE Controller
    // Class:       01          Mass storage controller (IDE Interface)
    // Subclass:      01        IDE controller (IDE Interface)

    // Subvendor:   8086        Intel Corporation
    // Subdevice:   d620        Device

    pcidev *ide = find_dev(0x8086, 0x27df, 0x01, 0x01);
    //pcidev *ide = find_dev(0x8086, 0x24db, 0x01, 0x01);
    if (ide == (void*) 0) {
        return -1;
    }

#ifdef _ata_debug_
    c_printf("[ata.c][_ata_modinit]: IDE Contrller @= %x, irq: %x, Bar0: %x, Bar1: %x, Bar2: %x, Bar3: %x, Bar4 %x\n", ide->address, ide->irq, ide->bar0, ide->bar1, ide->bar2, ide->bar3, ide->bar4);
#endif

    ide_initialize(0x1f0, 0x3f6, 0x170, 0x376, 0x000);
    dev_summary();
    rw_test();

    return 0;
}

uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t result = 0;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x82);
    if (reg < 0x08)
        result = __inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = __inb(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        result = __inb(channels[channel].ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        result = __inb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 2);
    return result;
}


void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x82);
    if (reg < 0x08)
        __outb(channels[channel].base  + reg - 0x00, data);
    else if (reg < 0x0C)
        __outb(channels[channel].base  + reg - 0x06, data);
    else if (reg < 0x0E)
        __outb(channels[channel].ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        __outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 2);
}

void ide_read_bufb(uint8_t channel, uint8_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inb(channels[channel].base);
    }
}

void ide_read_bufw(uint8_t channel, uint16_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inw(channels[channel].base);
    }
}

void ide_read_bufl(uint8_t channel, uint32_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inl(channels[channel].base);
    }
}

//void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer,
//        unsigned int quads) {
//    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
//     *           ESP for all of the code the compiler generates between the inline
//     *           assembly blocks.
//     */
//    if (reg > 0x07 && reg < 0x0C)
//        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].interrupt);
//    __asm("pushw %es; movw %ds, %ax; movw %ax, %es");
//    if (reg < 0x08)
//        __inl(channels[channel].base  + reg - 0x00, buffer, quads);
//    else if (reg < 0x0C)
//        __inl(channels[channel].base  + reg - 0x06, buffer, quads);
//    else if (reg < 0x0E)
//        __inl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
//    else if (reg < 0x16)
//        __inl(channels[channel].bmide + reg - 0x0E, buffer, quads);
//    __asm("popw %es;");
//    if (reg > 0x07 && reg < 0x0C)
//        ide_write(channel, ATA_REG_CONTROL, channels[channel].interrupt);
//}

uint8_t ide_polling(uint8_t channel, uint32_t advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for(int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.

}

unsigned char ide_print_error(unsigned int drive, unsigned char err) {
    if (err == 0)
        return err;

    c_printf("IDE:");
    if (err == 1) {c_printf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)   {c_printf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ATA_ER_TK0NF)  {c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_ABRT)   {c_printf("- Command Aborted\n     ");      err = 20;}
        if (st & ATA_ER_MCR)    {c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_IDNF)   {c_printf("- ID mark not Found\n     ");      err = 21;}
        if (st & ATA_ER_MC)     {c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_UNC)    {c_printf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ATA_ER_BBK)    {c_printf("- Bad Sectors\n     ");       err = 13;}
    } else  if (err == 3)           {c_printf("- Reads Nothing\n     "); err = 23;}
    else  if (err == 4)  {c_printf("- Write Protected\n     "); err = 8;}
    c_printf("- [%s %s] %s\n",
            (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
            (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
            ide_devices[drive].model);

    return err;
}

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3,
        unsigned int BAR4) {

    int device_count = 0;

    // 2- Disable IRQs:
    ide_write(ATA_MASTER, ATA_REG_CONTROL, 2);
    ide_write(ATA_SLAVE, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
    for (int dev = 0; dev < pci_dev_count; dev++) {
        pcidev *device = &pci_devs[dev];

        if (device->classid == (uint8_t) 0x01) {

            // configure base I/O port, control, and other registers
            channels[ATA_MASTER].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
            channels[ATA_MASTER].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
            channels[ATA_SLAVE].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
            channels[ATA_SLAVE].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
            channels[ATA_MASTER].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
            channels[ATA_SLAVE].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

            ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
            ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

            for (int chan = 0; chan < 2; chan++) {
                for (int dev_on_chan = 0; dev_on_chan < 2; dev_on_chan++) {
                    uint8_t err = 0;
                    uint8_t type = ATA_TYPE_ATA;
                    uint8_t status;

                    ide_devices[device_count].reserved = 0; // Assuming that there is no drive

                    // (I) Select drive
                    ide_write(chan, ATA_REG_HDDEVSEL, 0xa0 | (dev_on_chan << 4) );
                    __delay(100);

                    // (II) Send ATA identify command
                    ide_write(chan, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
                    __delay(100);

                    // if status is 0, there is not device, so stop configuring
                    if (ide_read(chan, ATA_REG_STATUS) == 0) {
                        continue;
                    }

                    // (III) Polling
                    while (1) {
                        status = ide_read(chan, ATA_REG_STATUS);

                        if (status & ATA_SR_ERR) {
                            err = 1;        // if err, device is not ATA
                            c_printf("\nerror\n");
                            break;
                        }

                        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                            c_printf("\nready\n");
                            break;          // everything is okay
                        }

                        //c_printf("Error status is %d\nstill in poll\n", err);

                    }
                    c_printf("check polling status");

                    // (IV) Probe for ATAPI devices
                    if (err != 0) {
                        uint8_t low = ide_read(chan, ATA_REG_LBA1);
                        uint8_t high = ide_read(chan, ATA_REG_LBA2);

                        if (low == 0x14 && high == 0xEB)
                            type = ATA_TYPE_ATAPI;
                        else if (low == 0x69 && high == 0x96)
                            type = ATA_TYPE_ATAPI;
                        else
                            continue;

                        ide_write(chan, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                        __delay(1);
                    }

                    // (V) Read identification space of the device
                    //
                    c_printf("Read identification space");
                    //ide_read_buffer(chan, ATA_REG_DATA, (uint32_t *)ide_buf, 128);
                    ide_read_bufl(chan, (uint32_t *)ide_buf, 128);

                    c_printf("read buffer 32!");

                    // (VI) Read device parameters
                    ide_devices[device_count].reserved     = 1;
                    ide_devices[device_count].type         = type;
                    ide_devices[device_count].channel      = chan;
                    ide_devices[device_count].drive        = dev_on_chan;
                    ide_devices[device_count].signature    = *((uint16_t *)(ide_buf + ATA_IDENT_DEVICETYPE));
                    ide_devices[device_count].capabilities = *((uint16_t *)(ide_buf + ATA_IDENT_CAPABILITIES));
                    ide_devices[device_count].commandSets  = *((uint32_t *)(ide_buf + ATA_IDENT_COMMANDSETS));

                    // (VII) Get addressing size
                    if (ide_devices[device_count].commandSets & (1 << 26)) {
                        // Device uses 48-Bit Addressing:
                        ide_devices[device_count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
                        c_printf("get size vii 48 bit");
                    }
                    else {
                        // Device uses CHS or 28-bit Addressing:
                        ide_devices[device_count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));
                        c_printf("get size vii chs or 28 bit bit");
                    }

                    c_printf("get size vii");

                    // (VIII) Get model of device
                    for(int str = 0; str < 40; str += 2) {
                        ide_devices[device_count].model[str] = ide_buf[ATA_IDENT_MODEL + str + 1];
                        ide_devices[device_count].model[str + 1] = ide_buf[ATA_IDENT_MODEL +  str];
                    }

                    ide_devices[device_count].model[40] = 0; // Terminate String.

                    c_printf("%s", ide_devices[device_count].model);

                    c_printf("get model of device");

                    device_count++;
                }
            }
            // stop looking if PATA is found
            break;
        }
        else {
            continue;
        }
    }
}

void dev_summary() {
    // 4- Print Summary:
    // for ever possible IDE device
    for (int dev = 0; dev < 4; dev++) {
        // if IDE device is valid
        if (ide_devices[dev].reserved == 1) {
            c_printf("Detected %s device w/ %dMB: %s\n",
                    (const char *[]){"ATA", "ATAPI"}[ide_devices[dev].type],
                    ide_devices[dev].size / 1024 / 2,
                    ide_devices[dev].model);
            c_printf("    Signature:    %04x\n", ide_devices[dev].signature);
            c_printf("    Capabilities: %04x\n", ide_devices[dev].capabilities);
            c_printf("    CommandSets:  %08x\n", ide_devices[dev].commandSets);
        }
    }
}

// We are only going to do 28-bit LBA because we don't need hard drives larger
// than 128GB and all hard drives support this mode
int ata_pio_rw(struct ide_device *dev, uint32_t sector, uint8_t *buffer, uint32_t bytes, enum pio_direction rw) {
    enum lba_support addressing;

    // disable interrupts on all drives in this channel
    ide_write(dev->channel, ATA_REG_CONTROL, channels[dev->channel].interrupt = (ide_irq_invoked = 0x0) + 0x02);

    // (I) Select one from LBA48, LBA 28, or CHS
    // most modern hard disks support LBA48 mode
    // all devices support LBA28 mode
    // we are not doing CHS mode -- it has some weird bit shifting
    if (dev->commandSets & (1 << 26)) {
        addressing = LBA48;
    }
    else if (dev->capabilities & 0x200) {
        addressing = LBA28;
    }
    //else {
    //    addressing = CHS;
    //}

    // (II) See if drive supports DMA or not
    // we do not support DMA
    //dma = 0;

    // (III) Wait if the drive is busy
    while (ide_read(dev->channel, ATA_REG_STATUS) & ATA_SR_BSY);

    // (IV) Select drive from the controller
    // we do not support CHS mode, so just do LBA
    ide_write(dev->channel, ATA_REG_HDDEVSEL, 0xE0 | (dev->drive << 4) | ((sector >> 24) & 0x0F));

    //if (addressing = CHS) {
    //    ide_write(dev->channel, ATA_REG_HDDEVSEL, 0xA0 | (dev->drive << 4) | head);
    //} else {
    //}

    // (V) Write parameters and configure LBA
    ide_write(dev->channel, ATA_REG_SECCOUNT0, 1);
    ide_write(dev->channel, ATA_REG_LBA0, (sector & 0xff));
    ide_write(dev->channel, ATA_REG_LBA1, (sector & 0xff00) >> 8);
    ide_write(dev->channel, ATA_REG_LBA2, (sector & 0x0ff0000) >> 16);
    if (addressing == LBA48) {
        ide_write( dev->channel, ATA_REG_SECCOUNT1, 1 );
        ide_write( dev->channel, ATA_REG_LBA3, (sector & 0xff000000) >> 24);
        ide_write( dev->channel, ATA_REG_LBA4, 0 );
        ide_write( dev->channel, ATA_REG_LBA5, 0 );
    }

    // (VI) Select the command and send it
    // osdev has a variety of command sets to pick from
    // becuase their function does both in and out
    // and attempts to account for DMA
    int command = 0x00;
    if ((addressing == LBA48) && (rw == READ)) {
        command = ATA_CMD_READ_PIO_EXT;
    } else if ((addressing == LBA28) && (rw == READ)) {
        command = ATA_CMD_READ_PIO;
    } else if ((addressing == LBA48) && (rw == WRITE)) {
        command = ATA_CMD_WRITE_PIO_EXT;
    } else if ((addressing == LBA28) && (rw == WRITE)) {
        command = ATA_CMD_WRITE_PIO;
    }
    ide_write(dev->channel, ATA_REG_COMMAND, command);

    // after sending command, we should poll, then read/write a sector, then poll,
    // then read/write a sector and rinse-repeat until we are done
    // also, we will catch errors

    // for loop for number of bytes to read

    int status = ide_polling(dev->channel, 1);                          // poll and check status of device
    while (!(ide_read(dev->channel, ATA_REG_STATUS) & ATA_SR_DRQ)); // while data request not ready

    // if there is no device error
    if (!status) {
        if (rw == READ) {
            uint16_t data;
            int bytestoread = 0;
            int numzeroes = 0;

            if (bytes % 2 == 1) {
                //  pad odd number of bytes with zeroes
                bytestoread = (bytes + 1)/2;
            } else {
                bytestoread = bytes/2;
            }

            numzeroes = 256 - bytestoread;

            // read in bytes into data and write to buffer
            for (int i = 0; i < bytestoread; i++) {
                data = __inw( channels[dev->channel].base );
                (buffer)[2*i] = (uint8_t)data;
                (buffer)[(2*i)+1] = (uint8_t)(data >> 8);
            }

            // pad for as many zeroes at we need to
            for (int i = 0; i < numzeroes; i++) {
                __inw( channels[dev->channel].base );
                (buffer)[2*i] = 0x00;
                (buffer)[(2*i)+1] = 0x00;
            }
        } else if (rw == WRITE) {
            int bytestowrite = 0;
            int numzeroes = 0;

            if (bytes % 2 == 1) {
                //  pad odd number of bytes with zeroes
                bytestowrite = (bytes + 1)/2;
            } else {
                bytestowrite = bytes/2;
            }

            numzeroes = 256 - bytestowrite;

            // write bytes out to device
            for (int i = 0; i < bytestowrite; i++) {
                __outw(channels[dev->channel].base, buffer[i]);
            }

            // pad zeroes
            for (int i = 0; i < numzeroes; i++) {
                __outw(channels[dev->channel].base, 0);
            }
        }

    }
    else {
        return status;
        c_printf("device busy, %s failed", rw);
    }

    return 0;
}

int read_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf) {
    return disk_read(dev, sector, buf, 512);
}

int disk_read(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes) {
    return ata_pio_rw(dev, sector, buf, bytes, READ);
}

int write_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf) {
    return disk_write(dev, sector, buf, 512);
}

int disk_write(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes) {
    return ata_pio_rw(dev, sector, buf, bytes, WRITE);
}

void rw_test() {
    uint8_t data[512];
    char *string = "welcome 1 2 3\n";   // interesting --
                                        // alpha characters are sized as 1 unit
                                        // spaces as 4
                                        // numbers as 2

    for (int dev = 0; dev < 4; dev++) {
        // if IDE device is valid
        if (ide_devices[dev].reserved == 1) {
            c_printf("device #%d\n", dev);
            disk_write(&ide_devices[dev], 1, (uint8_t *)string, 48);
            //c_printf("finished writing\n");

            read_sector(&ide_devices[dev], 1, data);
            for (int i = 0; i < 26; i++ ) {
                c_printf("%d-%c\n", i, data[i]);
                data[i] = 0;
            }
        }
    }

    _kpanic("rw_test", "read write finished?!?");
}

//void ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba,
//        unsigned short es, unsigned int edi) {
//
//    int error = 0x00;
//
//    // 1: Check if the drive presents:
//    // ==================================
//    if (drive > 3 || ide_devices[drive].reserved == 0) error = 0x1;      // Drive Not Found!
//
//    // 2: Check if inputs are valid:
//    // ==================================
//    else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == ATA_TYPE_ATA))
//        error = 0x2;                     // Seeking to invalid position.
//
//    // 3: Read in PIO Mode through Polling & IRQs:
//    // ============================================
//    else {
//        //unsigned char err;
//        if (ide_devices[drive].type == ATA_TYPE_ATA)
//            error = ide_ata_access(ATA_READ, drive, lba, numsects, es, edi);
//        else if (ide_devices[drive].type == ATA_TYPE_ATAPI)
//            for (int i = 0; i < numsects; i++)
//                error = ide_atapi_read(drive, lba + i, 1, es, edi + (i*2048));
//        error = ide_print_error(drive, error);
//    }
//}
