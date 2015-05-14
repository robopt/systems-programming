#ifndef _ATA_H_
#define _ATA_H_

// Definitions from http://www.ata-atapi.com/atadrvr.html
// and https://github.com/scialex/reenix/blob/vfs/kernel/drivers/blockdev/disk/ata.rs

// Command registers
#define    DATA         0x00    // data register    in/out    pio_base_addr1+0
#define    ERROR        0x01    // error            in        pio_base_addr1+1
#define    FEATURE      0x01    // feature             out    pio_base_addr1+1
#define    SECCOUNT     0x02    // sector count     in/out    pio_base_addr1+2
#define    SECNUM       0x03    // sector number    in/out    pio_base_addr1+3
#define    CYLOW        0x04    // cylinder low     in/out    pio_base_addr1+4
#define    CYHIGH       0x05    // cylinder high    in/out    pio_base_addr1+5
#define    DRIVEHEAD    0x06    // device head      in/out    pio_base_addr1+6
#define    STATUS       0x07    // primary status   in        pio_base_addr1+7
#define    CMD          0x07    // command             out    pio_base_addr1+7

#define    ASTATUS      0x08    // alternate status in        pio_base_addr2+6
#define    DRIVECTRL    0x08    // device control      out    pio_base_addr2+6
#define    DRIVEADDR    0x09    // device address   in        pio_base_addr2+7

// Error codes
#define    ER_ICRC      0x80    // ATA Ultra DMA bad CRC
#define    ER_BBK       0x80    // ATA bad block
#define    ER_UNC       0x40    // ATA uncorrected error
#define    ER_MC        0x20    // ATA media change
#define    ER_IDNF      0x10    // ATA id not found
#define    ER_MCR       0x08    // ATA media change request
#define    ER_ABRT      0x04    // ATA command aborted
#define    ER_NTK0      0x02    // ATA track 0 not found
#define    ER_NDAM      0x01    // ATA address mark not found

int _ata_modinit(void);

#endif
