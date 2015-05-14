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

unsigned char ide_read_reg(unsigned char channel, unsigned char reg) {
    unsigned char result = 0;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = __inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = __inb(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        result = __inb(channels[channel].control  + reg - 0x0A);
    else if (reg < 0x16)
        result = __inb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, channels[channel].nIEN);
    return result;
}

void ide_write_reg(unsigned char channel, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        __outb(channels[channel].base  + reg - 0x00, data);
    else if (reg < 0x0C)
        __outb(channels[channel].base  + reg - 0x06, data);
    else if (reg < 0x0E)
        __outb(channels[channel].control  + reg - 0x0A, data);
    else if (reg < 0x16)
        __outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, channels[channel].nIEN);
}

void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer,
        unsigned int quads) {
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
     *           ESP for all of the code the compiler generates between the inline
     *           assembly blocks.
     */
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, 0x80 | channels[channel].nIEN);
    __asm("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08)
        __insl(channels[channel].base  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        __insl(channels[channel].base  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        __insl(channels[channel].control + reg - 0x0A, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    __asm("popw %es;");
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, REG_DRIVECTRL, channels[channel].nIEN);
}

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for(int i = 0; i < 4; i++)
        ide_read_reg(channel, REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read_reg(channel, REG_STATUS) & SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        unsigned char state = ide_read_reg(channel, REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.

}

unsigned char ide_print_error(unsigned int drive, unsigned char err) {
    if (err == 0)
        return err;

    printf("IDE:");
    if (err == 1) {printf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        unsigned char st = ide_read_reg(ide_devices[drive].channel, REG_ERROR);
        if (st & ER_NDAM)   {printf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ER_NTK0)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ER_ABRT)   {printf("- Command Aborted\n     ");      err = 20;}
        if (st & ER_MCR)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ER_IDNF)   {printf("- ID mark not Found\n     ");      err = 21;}
        if (st & ER_MC)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ER_UNC)   {printf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ER_BBK)   {printf("- Bad Sectors\n     ");       err = 13;}
    } else  if (err == 3)           {printf("- Reads Nothing\n     "); err = 23;}
    else  if (err == 4)  {printf("- Write Protected\n     "); err = 8;}
    printf("- [%s %s] %s\n",
            (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
            (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
            ide_devices[drive].model);

    return err;
}

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3,
        unsigned int BAR4) {

    int j, i, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_MASTER  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_MASTER  ].control  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SLAVE].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SLAVE].control  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_MASTER  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SLAVE].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

    // 2- Disable IRQs:
    ide_write(ATA_MASTER  , REG_DRIVECTRL, 2);
    ide_write(ATA_SLAVE, REG_DRIVECTRL, 2);

    // 3- Detect ATA-ATAPI Devices:
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {

            unsigned char err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, REG_DRIVEHEAD, 0xA0 | (j << 4)); // Select Drive.
            sleep(1); // Wait 1ms for drive select to work.

            // (II) Send ATA Identify Command:
            ide_write(i, REG_CMD, CMD_IDENTIFY);
            sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
            // it is based on System Timer Device Driver.

            // (III) Polling:
            if (ide_read(i, REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while(1) {
                status = ide_read(i, REG_STATUS);
                if ((status & SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & SR_BSY) && (status & SR_DRQ)) break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:

            if (err != 0) {
                unsigned char cl = ide_read(i, REG_LBA1);
                unsigned char ch = ide_read(i, REG_LBA2);

                if (cl == 0x14 && ch ==0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                ide_write(i, REG_CMD, CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, REG_DATA, (unsigned int) ide_buf, 128);

            // (VI) Read Device Parameters:
            ide_devices[count].reserved     = 1;
            ide_devices[count].type         = type;
            ide_devices[count].channel      = i;
            ide_devices[count].drive        = j;
            ide_devices[count].signature    = *((unsigned short *)(ide_buf + IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((unsigned short *)(ide_buf + IDENT_CAPABILITIES));
            ide_devices[count].commandSets  = *((unsigned int *)(ide_buf + IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].commandSets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].size   = *((unsigned int *)(ide_buf + IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].size   = *((unsigned int *)(ide_buf + IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for(int k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[IDENT_MODEL + k];}
            ide_devices[count].model[40] = 0; // Terminate String.

            count++;
        }

    // 4- Print Summary:
    for (i = 0; i < 4; i++)
        if (ide_devices[i].reserved == 1) {
            printf(" Found %s Drive %dGB - %s\n",
                    (const char *[]){"ATA", "ATAPI"}[ide_devices[i].type],         /* Type */
                    ide_devices[i].size / 1024 / 1024 / 2,               /* Size */
                    ide_devices[i].model);
        }
}

