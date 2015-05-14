#ifndef _ATA_H_
#define _ATA_H_

#include <types.h>

// Definitions from http://www.ata-atapi.com/atadrvr.html
// and https://github.com/scialex/reenix/blob/vfs/kernel/drivers/blockdev/disk/ata.rs

// IDE Type
#define     IDE_ATA     0x00
#define     IDE_ATAPI   0x01

// Master or slave
#define     ATA_MASTER  0x00
#define     ATA_SLAVE   0x01

// Register offsets
#define     REG_DATA        0x00    // data register    in/out  BAR0 + 0
#define     REG_ERROR       0x01    // error            in      BAR0 + 1
#define     REG_FEATURES    0x01    // feature             out  BAR0 + 1
#define     REG_SECCOUNT0   0x02    // sector count     in/out  BAR0 + 2
#define     REG_SECNUM      0x03    // sector number    in/out  BAR0 + 3
#define     REG_CYLOW       0x04    // cylinder low     in/out  BAR0 + 4
#define     REG_CYHIGH      0x05    // cylinder high    in/out  BAR0 + 5
#define     REG_DRIVEHEAD   0x06    // device head      in/out  BAR0 + 6
#define     REG_STATUS      0x07    // primary status   in      BAR0 + 7
#define     REG_CMD         0x07    // command             out  BAR0 + 7

#define     REG_LBA0        0x03    // LBA0             in/out  BAR0 + 3
#define     REG_LBA1        0x04    // LBA1             in/out  BAR0 + 4
#define     REG_LBA2        0x05    // LBA2             in/out  BAR0 + 5

// Used only in LBA48
#define     REG_SECCOUNT1   0x08
#define     REG_LBA3        0x09
#define     REG_LBA4        0x0a
#define     REG_LBA5        0x0b

#define     ASTATUS     0x08    // alternate status     in      BAR1 + 6
#define     DRIVECTRL   0x08    // device control          out  BAR1 + 6
#define     DRIVEADDR   0x09    // device address       in      BAR1 + 7

// Error codes
#define     ER_ICRC     0x80    // ATA Ultra DMA bad CRC
#define     ER_BBK      0x80    // ATA bad block
#define     ER_UNC      0x40    // ATA uncorrected error
#define     ER_MC       0x20    // ATA media change
#define     ER_IDNF     0x10    // ATA id not found
#define     ER_MCR      0x08    // ATA media change request
#define     ER_ABRT     0x04    // ATA command aborted
#define     ER_NTK0     0x02    // ATA track 0 not found
#define     ER_NDAM     0x01    // ATA address mark not found

// Commands
#define     CMD_READ_PIO            0x20
#define     CMD_READ_PIO_EXT        0x24
#define     CMD_READ_DMA            0xC8
#define     CMD_READ_DMA_EXT        0x25
#define     CMD_WRITE_PIO           0x30
#define     CMD_WRITE_PIO_EXT       0x34
#define     CMD_WRITE_DMA           0xCA
#define     CMD_WRITE_DMA_EXT       0x35
#define     CMD_CACHE_FLUSH         0xE7
#define     CMD_CACHE_FLUSH_EXT     0xEA
#define     CMD_PACKET              0xA0
#define     CMD_IDENTIFY_PACKET     0xA1
#define     CMD_IDENTIFY            0xEC

// Defined buffer bytes in "identification space" from cmd identify and identify packet
#define     ATA_IDENT_DEVICETYPE    0
#define     ATA_IDENT_CYLINDERS     2
#define     ATA_IDENT_HEADS         6
#define     ATA_IDENT_SECTORS       12
#define     ATA_IDENT_SERIAL        20
#define     ATA_IDENT_MODEL         54
#define     ATA_IDENT_CAPABILITIES  98
#define     ATA_IDENT_FIELDVALID    106
#define     ATA_IDENT_MAX_LBA       120
#define     ATA_IDENT_COMMANDSETS   164
#define     ATA_IDENT_MAX_LBA_EXT   200

struct IDEChannelRegisters {
    uint32_t base;      // I/O channel
    uint32_t control;   // Control I/O channel
    uint32_t bmide;     // Bus master IDE for DMA
    uint32_t nIEN;      // nIEN (no interrupt)
} channels[2];

unsigned char ide_buf[2048] = {0};
unsigned static char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct ide_device {
    unsigned char  Reserved;    // 0 (Empty) or 1 (This Drive really exists).
    unsigned char  Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
    unsigned char  Drive;       // 0 (Master Drive) or 1 (Slave Drive).
    unsigned short Type;        // 0: ATA, 1:ATAPI.
    unsigned short Signature;   // Drive Signature
    unsigned short Capabilities;// Features.
    unsigned int   CommandSets; // Command Sets Supported.
    unsigned int   Size;        // Size in Sectors.
    unsigned char  Model[41];   // Model in string.
} ide_devices[4];

int _ata_modinit(void);
unsigned char ide_read(unsigned char channel, unsigned char reg);
void ide_write(unsigned char channel, unsigned char reg, unsigned char data);

#endif
