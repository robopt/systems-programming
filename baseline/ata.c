/*
 *  File:           ata.c
 *
 *  Author:         Stanley Chan, Claire Charron, Ed Mead
 *                  Credit to OSDev wiki (http://wiki.osdev.org/PCI_IDE_Controller
 *                  and http://wiki.osdev.org/ATA_PIO_Mode#Detection_and_Initialization),
 *                  Reenix (https://github.com/scialex/reenix), code from DOSS 
 *                  (https://github.com/agargiulo/DOSS/tree/master/disk), and
 *                  ATA-ATAPI.COM (http://www.ata-atapi.com/atadrvr.html) for
 *                  providing huge amount of resources, documentation, and help in making this driver
 *
 *  Description:    ATA driver to perform configuration, read, and write operations
 *                  on PATA hard disks.
*/

#include "ata.h"
#include "klib.h"
#include "pci.h"
#include "support.h"
#include "ulib.h"
#include "x86arch.h"

//#define _ata_debug_
#ifdef  _ata_debug_
#include "c_io.h"
#endif

/*
 *  Name:           static struct ata_channel channels[2]
 *
 *  Description:    struct declaration for ATA channel. Each motherboard can
 *                  support up to two ATA channels. Contains info on which port
 *                  is used for commands and control, as well as bus master IDE.
 *                  Note that we do not use the bus master IDE for functionality.
*/
static struct ata_channel {
    uint32_t base;          // base port for cmd registers
    uint32_t ctrl;          // base port for ctrl registers
    uint32_t bmide;         // bus master IDE
    uint8_t  interrupt;
} channels[2];

/*
 *  Name:           struct ide_device ide_devices[4]
 *
 *  Description:    struct declaration for the detected IDE device. Contains
 *                  configuration about the device, such as whether it exists,
 *                  which channel it is on, master/ slave setting, and more.
*/
struct ide_device {
    uint8_t reserved;       // 0 (Empty) or 1 (This Drive really exists).
    uint8_t channel;        // 0 (Primary Channel) or 1 (Secondary Channel).
    uint8_t drive;          // 0 (Master Drive) or 1 (Slave Drive).
    uint16_t type;          // 0: ATA, 1:ATAPI.
    uint16_t signature;     // Drive Signature
    uint16_t cylinders;     // Maxtor 6E040L0 has 16383
    uint16_t heads;         // Maxtor 6E040L0 has 16
    uint16_t sectors;       // Maxtor 6E040L0 has 63
    uint16_t serial;
    uint16_t capabilities;  // Features.
    uint32_t fieldvalid;
    uint32_t max_lba;       // Maxtor 6E040L0 should be 80293248
    uint32_t commandSets;   // Command Sets Supported.
    uint32_t max_lba_ext;   // Only useful with drives > 2TB
    uint32_t size;          // Size in Sectors.
    uint8_t model[41];      // Model in string.
} ide_devices[4];

uint8_t ide_buf[2048] = {0};    // buffer used to read the identification space
                                // and fill the ide_device struct

unsigned char ide_irq_invoked = 0;  // used to disable all channel interrupts
                                    // from connected drives

/*
 *  Name:           _ata_modinit()
 *
 *  Description:    Finds the attached device on the PCI bus, printing out it's
 *                  info, initializes the drives connected to the controller,
 *                  prints out the device summary, and writes a string to
 *                  the disk
 *
 * Arguments:       None (void)
 *
 * Return:          int: 0 if IDE controller was located
 *                      -1 otherwise
*/
int _ata_modinit() {
    // Slot:        00:1f.1

    // Vendor:      8086        Intel Corporation
    // Device:      27df        82801G (ICH7 Family) IDE Controller
    // Class:       01          Mass storage controller (IDE Interface)
    // Subclass:      01        IDE controller (IDE Interface)

    // Subvendor:   8086        Intel Corporation
    // Subdevice:   d620        Device

    pcidev *ide = find_dev(0x8086, 0x27df, 0x01, 0x01);

    // device for older DSL computers
    //pcidev *ide = find_dev(0x8086, 0x24db, 0x01, 0x01);

    if (ide == (void*) 0) {
        return -1;      // return -1 if IDE controllers were not found
    }

#ifdef _ata_debug_
    c_printf("[ata.c][_ata_modinit]: IDE Contrller @= %x, irq: %x, Bar0: %x, Bar1: %x, Bar2: %x, Bar3: %x, Bar4 %x\n", ide->address, ide->irq, ide->bar0, ide->bar1, ide->bar2, ide->bar3, ide->bar4);
#endif

    // initialize IDE drives
    ide_initialize( ATA_PRIMARY_CMD_BASE, ATA_PRIMARY_CTRL_BASE, ATA_SECONDARY_CMD_BASE, ATA_SECONDARY_CTRL_BASE, 0x000);

    dev_summary();      // print out drive summary
    rw_test();          // test driver by writing to and reading from disk

    return 0;
}

/*
 *  Name:           ide_read
 *
 *  Description:    Read the information contained in the IDE device registers.
 *                  Primarily useful for checking status codes, configuration,
 *                  and current state of the device.
 *
 *                  Note: This code was from primarily from the OSDev wiki page:
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *
 *  Arguments:      channel:    channel which the disk is located at
 *                  reg:        device register that we want to interrogate
 *
 *  Return:         uint8_t:    value read from the register
*/
uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t result = 0;

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x82);
        // 0x82 is the value of 0x80 and the interrupt disable
    }

    if (reg < 0x08) {
        result = __inb(channels[channel].base + reg - 0x00);
    } else if (reg < 0x0C) {
        result = __inb(channels[channel].base  + reg - 0x06);
    } else if (reg < 0x0E) {
        result = __inb(channels[channel].ctrl  + reg - 0x0A);
    } else if (reg < 0x16) {
        result = __inb(channels[channel].bmide + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 2);
    }

    return result;
}

/*
 *  Name:           ide_write
 *
 *  Description:    Write directly to the IDE device registers. Helps configure
 *                  the device with specific settings, such as disabling
 *                  interrupts and such.
 *
 *                  Note: This code was from primarily from the OSDev wiki page:
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *
 *  Arguments:      channel:    channel which the disk is located at
 *                  reg:        device register that we want to write to
 *
 *  Return:         nothing (void)
*/
void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x82);
        // 0x82 is the value of 0x80 and the interrupt disable
    }

    if (reg < 0x08) {
        __outb(channels[channel].base  + reg - 0x00, data);
    } else if (reg < 0x0C) {
        __outb(channels[channel].base  + reg - 0x06, data);
    } else if (reg < 0x0E) {
        __outb(channels[channel].ctrl  + reg - 0x0A, data);
    } else if (reg < 0x16) {
        __outb(channels[channel].bmide + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 2);
    }
}

/*
 *  Name:           ide_read_bufb
 *
 *  Description:    Read the configuration from device channel into the buffer.
 *                  This function is primarily used to read information from the
 *                  identification space.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *                  The OSDev code utilized assembly routines that we did not implement
 *                  and also had a caveat of trashing several registers.
 *                  The project at https://github.com/agargiulo/DOSS/tree/master/disk
 *                  improved upon this by splitting the read into different segment sizes.
 *                  This code was further cleaned up a bit by removing unnecessary parameters.
 *
 *  Arguments:      channel:    channel which the disk is located at
 *                  *buffer:    buffer to store data into
 *                  bufsize:    size of the buffer
 *
 *  Return:         nothing (void)
*/
void ide_read_bufb(uint8_t channel, uint8_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inb(channels[channel].base);
    }
}

/*
 *  Name:           ide_read_bufw
 *
 *  Description:    Read the configuration from device channel into the buffer.
 *                  This function is primarily used to read information from the
 *                  identification space.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *                  The OSDev code utilized assembly routines that we did not implement
 *                  and also had a caveat of trashing several registers.
 *                  The project at https://github.com/agargiulo/DOSS/tree/master/disk
 *                  improved upone this by splitting the read into different segment sizes.
 *                  This code was further cleaned up a bit by removing unnecessary parameters.
 *
 *  Arguments:      channel:    channel which the disk is located at
 *                  *buffer:    buffer to store data into
 *                  bufsize:    size of the buffer
 *
 *  Return:         nothing (void)
*/
void ide_read_bufw(uint8_t channel, uint16_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inw(channels[channel].base);
    }
}

/*
 *  Name:           ide_read_bufl
 *
 *  Description:    Read the configuration from device channel into the buffer.
 *                  This function is primarily used to read information from the
 *                  identification space.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *                  The OSDev code utilized assembly routines that we did not implement
 *                  and also had a caveat of trashing several registers.
 *                  The project at https://github.com/agargiulo/DOSS/tree/master/disk
 *                  improved upone this by splitting the read into different segment sizes.
 *                  This code was further cleaned up a bit by removing unnecessary parameters.
 *
 *  Arguments:      channel:    channel which the disk is located at
 *                  *buffer:    buffer to store data into
 *                  bufsize:    size of the buffer
 *
 *  Return:         nothing (void)
*/
void ide_read_bufl(uint8_t channel, uint32_t *buffer, int bufsize) {
    for (int i = 0; i < bufsize; i++) {
        buffer[i] = __inl(channels[channel].base);
    }
}

/*
 *  Name:           ide_polling
 *
 *  Description:    Poll for the status of the disk and check for errors. Primarily
 *                  used for initializing the disk and waiting for the device to be
 *                  ready after a command has been sent. The device should wait
 *                  approxiamately 400 nanoseconds before reading the status bits.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *                  It was slightly improved by always utilizing an advanced error
 *                  check -- the extra checks come at little to no cost.
 *
 *  Arguments:      channel:    channel which the disk is located at
 *
 *  Return:         uint8_t:    0 if the drive encountered no errors
 *                              1 if the drive encountered a fault
 *                              2 for generic error
 *                              3 for data request ready
*/
uint8_t ide_polling(uint8_t channel) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for(int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

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

    return 0; // No Error.

}

/*
 *  Name:           ide_print_error
 *
 *  Description:    Print information about the device error if an error was encountered.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *
 *  Arguments:      drive:      drive index in ide_devices array
 *                  err:        err status code read from register
 *
 *  Return:         uint8_t:    same as err value
*/
uint8_t ide_print_error(uint8_t drive, uint8_t err) {
    if (err == 0)
        return err;

    c_printf("IDE:");
    if (err == 1) {c_printf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) {c_printf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ATA_ER_TK0NF){c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_ABRT) {c_printf("- Command Aborted\n     ");      err = 20;}
        if (st & ATA_ER_MCR)  {c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_IDNF) {c_printf("- ID mark not Found\n     ");      err = 21;}
        if (st & ATA_ER_MC)   {c_printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_UNC)  {c_printf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ATA_ER_BBK)  {c_printf("- Bad Sectors\n     ");       err = 13;}
    } else  if (err == 3)           {c_printf("- Reads Nothing\n     "); err = 23;}
    else  if (err == 4)  {c_printf("- Write Protected\n     "); err = 8;}
    c_printf("- [%s %s] %s\n",
            (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
            (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
            ide_devices[drive].model);

    return err;
}

/*
 *  Name:           ide_initialize
 *
 *  Description:    Enumerate connected IDE drives and initialize and configure
 *                  them accordingly. This involves setting it's proper LBA mode
 *                  as well as initializing an entry in the ide_devices array.
 *                  Additionally, this is done via polling and does not have interrupts.
 *
 *                  Note: This code structure was from primarily from the OSDev
 *                  wiki page.
 *                  http://wiki.osdev.org/PCI_IDE_Controller#Detecting_IDE_Drives
 *                  The project at https://github.com/agargiulo/DOSS/tree/master/disk
 *                  modified the structure of this code and made it more readable.
 *                  Reenix also provided insignt into how the devices could be intialized:
 *                  https://github.com/scialex/reenix/tree/vfs/kernel/drivers/blockdev
 *                  The resulting function below also omits the ATAPI drive configuration
 *                  because that was not a goal of this project.
 *
 *  Arguments:      BAR0:       Base address of primary I/O channel
 *                  BAR1:       Base address of primarly I/O channel control port
 *                  BAR2:       Base address of secondary I/O channel
 *                  BAR3:       Base address of secondary I/O channel control port
 *                  BAR4:       Bus master IDE -- assists in DMA transfers, unused in this function
 *
 *  Return:         nothing (void)
*/
void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4) {

    int device_count = 0;

    // 1- Detect ATA-ATAPI Devices:

    // for every device detected when enumerating the PCI bus
    for (int dev = 0; dev < pci_dev_count; dev++) {
        pcidev *device = &pci_devs[dev];

        // and if the device is a PATA mass storage controller
        if (device->classid == (uint8_t) 0x01) {

            // 1- Detect I/O Ports which interface with the IDE Controller
            // configure base I/O port, control, and other registers
            channels[ATA_MASTER].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
            channels[ATA_MASTER].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
            channels[ATA_SLAVE].base   = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
            channels[ATA_SLAVE].ctrl   = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
            channels[ATA_MASTER].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
            channels[ATA_SLAVE].bmide  = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

            // 2- Disable IRQs:
            // OSDev wiki and other resources performed initialization primarily with polling
            ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
            ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

            // for every channel on the controller
            for (int chan = 0; chan < 2; chan++) {
                // and for every device located on the channel
                for (int dev_on_chan = 0; dev_on_chan < 2; dev_on_chan++) {
                    uint8_t err = 0;
                    uint8_t type = ATA_TYPE_ATA;
                    uint8_t status;

                    ide_devices[device_count].reserved = 0; // Drive does not exist

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
                    // wait until the device is ready for configuration
                    while (1) {
                        status = ide_read(chan, ATA_REG_STATUS);

                        if (status & ATA_SR_ERR) {
                            err = 1;        // if err, device is not ATA
                            //c_printf("\nerror\n");
                            break;
                        }

                        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                            //c_printf("\nready\n");
                            break;          // everything is okay
                        }

                        //c_printf("Error status is %d\nstill in poll\n", err);

                    }
                    //c_printf("check polling status");

                    // (IV) Probe for ATAPI devices
                    // even though we do not have ATAPI drivers, we still must
                    // configure them accordingly such that their entry on
                    // ide_devices is properly managed

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
                    ide_read_bufl(chan, (uint32_t *)ide_buf, 128);

                    // (VI) Read device parameters
                    ide_devices[device_count].reserved      = 1;
                    ide_devices[device_count].type          = type;
                    ide_devices[device_count].channel       = chan;
                    ide_devices[device_count].drive         = dev_on_chan;
                    ide_devices[device_count].signature     = *((uint16_t *)(ide_buf + ATA_IDENT_DEVICETYPE));
                    ide_devices[device_count].cylinders     = *((uint16_t *)(ide_buf + ATA_IDENT_CYLINDERS));
                    ide_devices[device_count].heads         = *((uint16_t *)(ide_buf + ATA_IDENT_HEADS));
                    ide_devices[device_count].sectors       = *((uint16_t *)(ide_buf + ATA_IDENT_SECTORS));
                    ide_devices[device_count].serial        = *((uint16_t *)(ide_buf + ATA_IDENT_SERIAL));
                    ide_devices[device_count].capabilities  = *((uint16_t *)(ide_buf + ATA_IDENT_CAPABILITIES));
                    ide_devices[device_count].fieldvalid    = *((uint32_t *)(ide_buf + ATA_IDENT_FIELDVALID));
                    ide_devices[device_count].max_lba       = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA));
                    ide_devices[device_count].commandSets   = *((uint32_t *)(ide_buf + ATA_IDENT_COMMANDSETS));
                    ide_devices[device_count].max_lba_ext   = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));

                    // (VII) Get addressing size
                    if (ide_devices[device_count].commandSets & (1 << 26)) {
                        // Device uses 48-Bit Addressing:
                        ide_devices[device_count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
                        //c_printf("get size vii 48 bit");
                    }
                    else {
                        // Device uses CHS or 28-bit Addressing:
                        ide_devices[device_count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));
                        //c_printf("get size vii chs or 28 bit bit");
                    }

                    //c_printf("get size vii");

                    // (VIII) Get model of device
                    for(int str = 0; str < 40; str += 2) {
                        ide_devices[device_count].model[str] = ide_buf[ATA_IDENT_MODEL + str + 1];
                        ide_devices[device_count].model[str + 1] = ide_buf[ATA_IDENT_MODEL +  str];
                    }

                    ide_devices[device_count].model[40] = 0; // Terminate String.

                    //c_printf("%s", ide_devices[device_count].model);

                    //c_printf("get model of device");

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

/*
 *  Name:           dev_summary
 *
 *  Description:    Prints out device details to the console.
 *
 *                  Note: this code structure was from primarily from the OSdev
 *                  wiki page.
 *
 *  Arguments:      nothing
 *
 *  Return:         nothing (void)
*/
void dev_summary() {
    // 4- Print Summary:
    // for ever possible IDE device
    for (int dev = 0; dev < 4; dev++) {
        // if IDE device is valid
        if (ide_devices[dev].reserved == 1) {

            // disk is divided into 512 byte sectors
            // dev->size returns max_lba, which is 2x the bytes (for the sector split), so divide by 2
            // divide by 1024 to get megabytes, again for gigabytes
            c_printf("%s device w/ %dGB: %s\n",
                    (const char *[]){"ATA", "ATAPI"}[ide_devices[dev].type],
                    ide_devices[dev].size / 1024 / 1024 / 2,
                    ide_devices[dev].model);
            c_printf("    Channel:      %04x\n", ide_devices[dev].channel);
            c_printf("    Drive:        %s\n",   (const char *[]){"MASTER", "SLAVE"}[ide_devices[dev].drive]);
            c_printf("    Type:         %s\n",   (const char *[]){"ATA", "ATAPI"}[ide_devices[dev].type]);
            c_printf("    Signature:    %04x\n", ide_devices[dev].signature);
            c_printf("    Capabilities: %04x\n", ide_devices[dev].capabilities);
            c_printf("    CommandSets:  %08x\n", ide_devices[dev].commandSets);
            c_printf("    Cylinders:    %d\n",   ide_devices[dev].cylinders);
            c_printf("    Heads:        %d\n",   ide_devices[dev].heads);
            c_printf("    Sectors:      %d\n",   ide_devices[dev].sectors);
            c_printf("    Serial:       %d\n",   ide_devices[dev].serial);
            c_printf("    FieldValid:   %08x\n", ide_devices[dev].fieldvalid);
            c_printf("    Max LBA:      %d\n",   ide_devices[dev].max_lba);
            c_printf("    Extended LBA: %d\n",   ide_devices[dev].max_lba_ext);

        }
    }
}

/*
 *  Name:           ata_pio_rw
 *
 *  Description:    Single function for Programmed I/O (PIO).
 *                  Disables interrupts on all devices in the channel and configures
 *                  the device for the proper LBA mode. It is important to note
 *                  that this driver does not support CHS mode or DMA transfers.
 *                  Additionally, both read and write functionality is provided
 *                  in this function and is specified via an enum rw type. Another
 *                  enum is also used to specify configuration for LBA48, LBA28, or CHS modes.
 *
 *                  Note: this code structure was from primarily from the OSdev
 *                  wiki page.
 *                  http://wiki.osdev.org/pci_ide_controller#detecting_ide_drives
 *                  The project at https://github.com/agargiulo/DOSS/tree/master/disk
 *                  improved upon this code by splitting the read/ write functionality
 *                  into two functions. However, since there was so much code duplication
 *                  in doing so, an enum was used to separate the functionality.
 *
 *  Arguments:      struct ide_device *dev: device from ide_devices to perform operation on
 *                  sector:                 sector of device to perform operation on
 *                  *buffer:                buffer to read and store into or write from
 *                  bytes:                  number of bytes to store
 *                  enum pio_directio rw:   enum for READ and WRITE operations
 *
 *
 *  Return:         int:    0 if no error, otherwise the error status returned from the register
*/
int ata_pio_rw(struct ide_device *dev, uint32_t sector, uint8_t *buffer, uint32_t bytes, enum pio_direction rw) {
    enum lba_support addressing;

    // disable interrupts on all drives in this channel
    ide_write(dev->channel, ATA_REG_CONTROL, channels[dev->channel].interrupt = (ide_irq_invoked = 0x0) + 0x02);

    // (I) Select one from LBA48, LBA 28, or CHS
    // most modern hard disks support LBA48 mode
    // all devices support LBA28 mode
    // we are not doing CHS mode -- it has some weird bit shifting that I do not understand...
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

    // (V) Write parameters and configure LBA mode
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
    // OSDev has a variety of command sets to pick from
    // because their function does both in and out
    // and attempts to account for DMA
    // since we do not do DMA or CHS, there are only 4 possible commands to send
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
    // also, errors encountered will be noted in status

    int status = ide_polling(dev->channel);                             // poll and check status of device
    while (!(ide_read(dev->channel, ATA_REG_STATUS) & ATA_SR_DRQ));     // while data request not ready

    // if there is no error
    if (!status) {
        // perform READ operation
        if (rw == READ) {
            uint16_t data;

            // read in bytes into data and write to buffer
            for (unsigned int i = 0; i < bytes; i++) {
                data = __inw( channels[dev->channel].base );
                (buffer)[i] = (uint8_t)data;
            }

            buffer[bytes] = 0;

        } else if (rw == WRITE) {
            // otherwise, perform WRITE operation

            int numzeroes = 0;

            if (bytes % 2 == 1) {
                //  pad odd number of bytes with zeroes
                numzeroes = 256 - (bytes + 1)/2;
            } else {
                numzeroes = 256 - bytes/2;
            }

            // write bytes out to device
            for (unsigned int i = 0; i < bytes; i++) {
                __outw(channels[dev->channel].base, buffer[i]);
            }

            // pad zeroes
            for (int i = 0; i < numzeroes; i++) {
                __outw(channels[dev->channel].base, 0);
            }
        }

    }
    // otherwise, an error has been encountered
    // print out details about it and return that error
    else {
        c_printf("device busy, %s failed", (const char *[]){"READ", "WRITE"}[rw]);
        ide_print_error(0, status);
        return status;
    }

    // no error, successful read/ write, return 0
    return 0;
}

/*
 *  Name:           read_sector
 *
 *  Description:    Wrapper around ata_pio_rw to read entire 512 bytes from the sector
 *
 *                  Code borrowed largely from https://github.com/agargiulo/DOSS/tree/master/disk
 *
 *  Arguments:      struct ide_device *dev: device from ide_devices to perform operation on
 *                  sector:                 sector of device to perform operation on
 *                  *buffer:                buffer to read from
 *
 *  Return:         int:    0 if no error, otherwise the error status returned from the register
*/
int read_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf) {
    return disk_read(dev, sector, buf, 512);
}

/*
 *  Name:           disk_read
 *
 *  Description:    Wrapper around ata_pio_rw to read a specified amount
 *                  of bytes from the sector
 *
 *                  Code borrowed largely from https://github.com/agargiulo/DOSS/tree/master/disk
 *
 *  Arguments:      struct ide_device *dev: device from ide_devices to perform operation on
 *                  sector:                 sector of device to perform operation on
 *                  *buffer:                buffer to read from
 *                  bytes:                  number of bytes to read
 *
 *  Return:         int:    0 if no error, otherwise the error status returned from the register
*/
int disk_read(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes) {
    return ata_pio_rw(dev, sector, buf, bytes, READ);
}

/*
 *  Name:           read_sector
 *
 *  Description:    Wrapper around ata_pio_rw to write entire 512 bytes to the sector
 *
 *                  Code borrowed largely from https://github.com/agargiulo/DOSS/tree/master/disk
 *
 *  Arguments:      struct ide_device *dev: device from ide_devices to perform operation on
 *                  sector:                 sector of device to perform operation on
 *                  *buffer:                buffer to write to
 *
 *  Return:         int:    0 if no error, otherwise the error status returned from the register
*/
int write_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf) {
    return disk_write(dev, sector, buf, 512);
}

/*
 *  Name:           disk_write
 *
 *  Description:    Wrapper around ata_pio_rw to write a specified amount
 *                  of bytes to the sector
 *
 *                  Code borrowed largely from https://github.com/agargiulo/DOSS/tree/master/disk
 *
 *  Arguments:      struct ide_device *dev: device from ide_devices to perform operation on
 *                  sector:                 sector of device to perform operation on
 *                  *buffer:                buffer to write to
 *                  bytes:                  number of bytes to write
 *
 *  Return:         int:    0 if no error, otherwise the error status returned from the register
*/
int disk_write(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes) {
    return ata_pio_rw(dev, sector, buf, bytes, WRITE);
}

/*
 *  Name:           rw_test
 *
 *  Description:    Small test function to check if write and read functionality
 *                  is working from the driver
 *
 *                  Code borrowed largely from https://github.com/agargiulo/DOSS/tree/master/disk
 *
 *  Arguments:      none
 *
 *  Return:         nothing (void)
*/
void rw_test() {
    uint8_t data[512];
    char *string = "testing in sector 4";   // interesting --
    // alpha characters are sized as 1 unit
    // spaces as 4
    // numbers as 2

    for (int dev = 0; dev < 4; dev++) {
        // if IDE device is valid
        if (ide_devices[dev].reserved == 1) {
            c_printf("Device #%d\n", dev);
            c_printf("    Writing out: \"%s\"\n", string);
            disk_write(&ide_devices[dev], 4, (uint8_t *)string, 19);
            //c_printf("finished writing\n");

            read_sector(&ide_devices[dev], 4, data);
            c_printf("    Read in:     \"");
            for (int i = 0; i < 19; i++ ) {
                c_printf("%c", data[i]);
                //c_printf("%d-%c ", i, data[i]);
                data[i] = 0;
            }
            c_printf("\"\n");
        }
    }

    //_kpanic("rw_test", "read write finished?!?");
}
