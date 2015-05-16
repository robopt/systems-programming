#ifndef _ATA_H_
#define _ATA_H_

#include <types.h>

// Definitions from http://www.ata-atapi.com/atadrvr.html
// and https://github.com/scialex/reenix/blob/vfs/kernel/drivers/blockdev/disk/ata.rs

/* Copied from the bochsrc, make sure it always matches */
#define IRQ_DISK_PRIMARY    14
#define IRQ_DISK_SECONDARY  15

#define ATA_NUM_CHANNELS    2
#define ATA_SECTOR_SIZE     512

// IDE Type
#define     ATA_TYPE_ATA    0x00
#define     ATA_TYPE_ATAPI  0x01

// Master or slave
#define     ATA_MASTER      0x00
#define     ATA_SLAVE       0x01

// Channels
#define ATA_PRIMARY         0x00
#define ATA_SECONDARY       0x01

// Operations
#define ATA_READ            0x00
#define ATA_WRITE           0x01

// drive head values
#define ATA_DRIVEHEAD_MASTER    0xa0
#define ATA_DRIVEHEAD_SLAVE     0xb0

/* Drive/head values for CHS / LBA */
#define ATA_DRIVEHEAD_CHS 0x00
#define ATA_DRIVEHEAD_LBA 0x40

// base port addresses
#define ATA_PRIMARY_CTRL_BASE   0x3f0
#define ATA_PRIMARY_CMD_BASE    0x1f0
#define ATA_SECONDARY_CTRL_BASE 0x370
#define ATA_SECONDARY_CMD_BASE  0x170

// Register offsets
#define     ATA_REG_DATA        0x00    // data register    in/out  BAR0 + 0
#define     ATA_REG_ERROR       0x01    // error            in      BAR0 + 1
#define     ATA_REG_FEATURES    0x01    // feature             out  BAR0 + 1
#define     ATA_REG_SECCOUNT0   0x02    // sector count     in/out  BAR0 + 2
#define     ATA_REG_SECNUM      0x03    // sector number    in/out  BAR0 + 3
#define     ATA_REG_CYLOW       0x04    // cylinder low     in/out  BAR0 + 4
#define     ATA_REG_CYHIGH      0x05    // cylinder high    in/out  BAR0 + 5
#define     ATA_REG_HDDEVSEL    0x06    // device head      in/out  BAR0 + 6
#define     ATA_REG_COMMAND     0x07    // command             out  BAR0 + 7
#define     ATA_REG_STATUS      0x07    // primary status   in      BAR0 + 7

#define     ATA_REG_LBA0        0x03    // LBA0             in/out  BAR0 + 3
#define     ATA_REG_LBA1        0x04    // LBA1             in/out  BAR0 + 4
#define     ATA_REG_LBA2        0x05    // LBA2             in/out  BAR0 + 5

// Used only in LBA48
#define     ATA_REG_SECCOUNT1   0x08
#define     ATA_REG_LBA3        0x09
#define     ATA_REG_LBA4        0x0a
#define     ATA_REG_LBA5        0x0b

#define     ATA_REG_CONTROL     0x0c    // device control          out  BAR1 + 6
#define     ATA_REG_ALTSTATUS   0x0c    // alternate status     in      BAR1 + 6
#define     ATA_REG_DRIVEADDR   0x0d    // device address       in      BAR1 + 7

// Status
#define ATA_SR_BSY      0x80    // Busy
#define ATA_SR_DRDY     0x40    // Drive ready
#define ATA_SR_DF       0x20    // Drive write fault
#define ATA_SR_DSC      0x10    // Drive seek complete
#define ATA_SR_DRQ      0x08    // Data request ready
#define ATA_SR_CORR     0x04    // Corrected data
#define ATA_SR_IDX      0x02    // Inlex
#define ATA_SR_ERR      0x01    // Error

// Error codes
#define     ATA_ER_ICRC     0x80    // ATA Ultra DMA bad CRC
#define     ATA_ER_BBK      0x80    // ATA bad block
#define     ATA_ER_UNC      0x40    // ATA uncorrected error
#define     ATA_ER_MC       0x20    // ATA media change
#define     ATA_ER_IDNF     0x10    // ATA id not found
#define     ATA_ER_MCR      0x08    // ATA media change request
#define     ATA_ER_ABRT     0x04    // ATA command aborted
#define     ATA_ER_TK0NF    0x02    // ATA track 0 not found
#define     ATA_ER_AMNF     0x01    // ATA address mark not found

// Commands
#define     ATA_CMD_READ_PIO            0x20
#define     ATA_CMD_READ_PIO_EXT        0x24
#define     ATA_CMD_READ_DMA            0xC8
#define     ATA_CMD_READ_DMA_EXT        0x25
#define     ATA_CMD_WRITE_PIO           0x30
#define     ATA_CMD_WRITE_PIO_EXT       0x34
#define     ATA_CMD_WRITE_DMA           0xCA
#define     ATA_CMD_WRITE_DMA_EXT       0x35
#define     ATA_CMD_CACHE_FLUSH         0xE7
#define     ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define     ATA_CMD_PACKET              0xA0
#define     ATA_CMD_IDENTIFY_PACKET     0xA1
#define     ATA_CMD_IDENTIFY            0xEC

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

extern unsigned char ide_buf[2048];
extern unsigned char ide_irq_invoked;

// forward declare struct
struct ide_device;
enum lba_support { LBA48, LBA28, CHS };
enum pio_direction { READ, WRITE };

// forward declare register byte operations
uint8_t __inb(uint8_t);
uint16_t __inw(uint8_t);
uint32_t __inl(uint8_t);
void __outb(uint8_t, uint8_t);
void __outw(uint8_t, uint16_t);

int _ata_modinit(void);
unsigned char ide_read(unsigned char channel, unsigned char reg);
void ide_write(unsigned char channel, unsigned char reg, unsigned char data);
void ide_read_bufb(uint8_t channel, uint8_t *buffer, int bufsize);
void ide_read_bufw(uint8_t channel, uint16_t *buffer, int bufsize);
void ide_read_bufl(uint8_t channel, uint32_t *buffer, int bufsize);
void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads);
uint8_t ide_polling(uint8_t channel, uint32_t advanced_check);
void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4);
void dev_summary(void);

int ata_pio_rw(struct ide_device *dev, uint32_t sectors, uint8_t *buffer, uint32_t bytes, enum pio_direction rw);
int read_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf);
int disk_read(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes);
int write_sector(struct ide_device *dev, uint32_t sector, uint8_t *buf);
int disk_write(struct ide_device *dev, uint32_t sector, uint8_t *buf, int bytes);

void rw_test(void);



#endif
