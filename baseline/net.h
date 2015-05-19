/* ==============================================================
** ===== Intel 8255x 10/100 Mbps Ethernet Controller Family =====
** ==============================================================
** 
** ===============
** = Terminology =
** ===============
** CSR - Control/Status Registers
**      \---> The CSR physically resides on the LAN controller and can be accesssed
**      \---> by either I/O or memory cycles, while the rest of the memory structures
**      \---> reside in system (host) memory. The first 8 bytes of the CSR is called
**      \---> the System Control Block (SCB).
** SCB - System Control Block
**      \---> The SCB serves as a central communication point for exchanging
**      \---> control and status information between the host CPU and the 8255x.
**      \---> Host controls the CU/RU by writing commands to the SCB.
** CU - Command Unit
** RU - Receive Unit
** CBL - Command Block List
** RFA - Recieve Frame Area
** 
**
** ======================================
** = Device Addressing Formatting Table =
** ================================================================================
** =   Points to   =   Base Register   =   32-bit Offset Pointer   =   Physical   =
** ================================================================================
** = Start of CBL  =  CU Base (32bit)  =   SCB General Pointer     =   Physical   =
** ================================================================================
** = Start of RFA  =  RU Base (32bit)  =   SCB General Pointer     =   Physical   =
** ================================================================================
** =    Next CB    =  CU Base (32bit)  =   Link Address in CBL     =   Physical   =
** ================================================================================
** = Start of TBD[]=  CU Base (32bit)  =   32-bit Offset Pointer   =   Physical   =
** ================================================================================
** =    Next RFD   =  RU Base (32bit)  =   32-bit Offset Pointer   =   Physical   =
** ================================================================================
** 
** Linear Addressing:
** Load a value of 00000000h into the CU base using the Load CU Base Address SCB command.
** Load a value of 00000000h into the RU base using the Load RU Base Address SCB command.
** Use the offset pointer values in the various data structures as absolute 32-bit linear addresses.
**
** 32-bit Segmented Addressing:
** Load the desired segment value into the CU base using the Load CU base Address SCB command.
** Load the desired segment value into the RU base using the Load RU base Address SCB command.
** Use the offset pointer values in the various data structures as 16-bit offsets. Software 
** must ensure that the upper 16 bits of this offset equal 0000h as the device will add all
** 32 bits of the base and offset values.
** 
** Controlling the Device
** Issue commands to CU/RU via SCB (which is part of the CSR)
** 
*/
/* SCB Status Word bit definitions */
#include "pci.h"

#ifndef _net_h
#define _net_h



#define SCB_CMD_EL              0x8000
#define SCB_CMD_S               0x4000

#define MAC_LEN                 6   //48bit mac addr
typedef struct net8255x_s {
    pcidev *pci;    //pci device
    uint32_t scb;   //status control block
    uint8_t mac[MAC_LEN]; //mac addr
} net8255x;

#define RFD_COUNT               16
#define RFD_DATA_SIZE           1496
#define RFD_HEAD_SIZE           14 // 2x Mac + protocol
typedef struct net_frame_s {
    uint8_t mac_dst[MAC_LEN];
    uint8_t mac_src[MAC_LEN];
    uint16_t proto;
    uint8_t data[RFD_DATA_SIZE];
} __attribute__((__packed__)) netframe;

typedef struct rfd_s {
    uint16_t status;    // status
    uint16_t command;   // command
    uint32_t link_addr; // link address offset
    uint32_t reserved;  // reserved
    uint16_t bytes_written;  
    uint16_t size;      // frame size
    netframe frame; // frame data
} __attribute__((__packed__)) rfd;

#define TXD_COUNT               16
//#define TXD_DATA_SIZE           1518 //1510?
#define TXD_DATA_SIZE           1510 //1510?
typedef struct txd_s {
    uint16_t status;    // status
    uint16_t command;   // command
    uint32_t link_addr; // link address offset
    
    uint32_t tx_buf_addr;
    uint16_t tx_count;
    uint8_t tx_thresh;
    uint8_t tx_buf_count;
    netframe frame;
} __attribute__((__packed__)) txd;

#define NET_CMD_DELAY           5

#define SCB_STATUS              0x00
#define SCB_STATACK             0x01 
#define SCB_COMMAND             0x02 
#define SCB_GEN_PTR             0x04
#define SCB_PORT                0x08
#define SCB_EEPROM              0x0C
#define SCB_SELF_TEST           0x0F
#define SCB_GEN_STATUS          0x1D
#define GEN_STATUS_LINK         0x01
#define GEN_STATUS_100          0x02
#define GEN_STATUS_DUPLEX       0x04

#define STATACK_MASK            0xFD
#define STATACK_CU_READY        0x80
#define STATACK_RU_FRAME        0x40
#define STATACK_CU_LEAVE_ACT    0x20
#define STATACK_RU_LEAVE_RDY    0x10
#define STATACK_MDI_RW          0x08
#define STATACK_SWI             0x04
#define STATACK_RESERVED        0x02
#define STATACK_FLOW_PAUSE      0x01

#define SCB_RUS_MASK            0x3C //00 1111 00
#define SCB_RUS_IDLE            0x00 //00 0000 00
#define SCB_RUS_SUSPEND         0x04 //00 0001 00
#define SCB_RUS_NORESOURCE      0x08 //00 0010 00
#define SCB_RUS_READY           0x10 //00 0100 00

#define SCB_CUS_MASK            0xC0 //11 0000 00
#define SCB_CUS_IDLE            0x00 //00 0000 00
#define SCB_CUS_SUSPEND         0x40 //01 0000 00
#define SCB_CUS_LPQ_ACTIVE      0x80 //10 0000 00
#define SCB_CUS_HQP_ACTIVE      0xB0 //11 0000 00

#define SCB_CU_NOOP             0x00 
#define SCB_CU_START            0x10
#define SCB_CU_RESUME           0x20
#define SCB_CU_START_HPQ        0x30
#define SCB_CU_LOAD_DUMP        0x40
#define SCB_CU_DUMP_COUNT       0x50
#define SCB_CU_LOAD_CU_BASE     0x60
#define SCB_CU_DUMP_RESET       0x07
#define SCB_CU_RESUME_STATIC    0x0A
#define SCB_CU_RESUME_HPQ       0x0B

#define SCB_RU_NOOP             0x00 
#define SCB_RU_START            0x01
#define SCB_RU_RESUME           0x02
#define SCB_RU_RESERVED         0x03
#define SCB_RU_ABORT            0x04
#define SCB_RU_LOAD_HDS         0x05
#define SCB_RU_LOAD_CU_BASE     0x06
#define SCB_RU_RBD_RESUME       0x07

/*
** Initialize 
** Return: 0 on Success, <0 on Error
*/
int _net_modinit(void);

/*
** Initialize the network card
** SW Reset and configuration
** Return: 0 on Success, <0 on Error
*/
int _net_modinit_driver(void);

void net_cmd_writeb(uint8_t offset, uint8_t cmd);
void net_cmd_writew(uint8_t offset, uint16_t cmd);
void net_cmd_writel(uint8_t offset, uint32_t cmd);
uint8_t net_cmd_readb(uint8_t offset);
uint16_t net_cmd_readw(uint8_t offset);
uint32_t net_cmd_readl(uint8_t offset);

/*
** Wait for the SCB Command to be accepted.
** Return: Number of spin-waits (multiply by NET_CMD_DELAY for tenths of seconds)
*/
int net_cmd_wait(void);

/*
** Write nbytes to the tx buffer
** Param [ buf ]: Buffer to write from
** Param [ nbytes ]: Number of bytes to write
** Return: Number of bytes written
*/
int net_write(char *buf, int nbytes);

/*
** Read up to n bytes, place into buffer.
** Param [ buf ]: Buffer to write to
** Param [ nbytes ]: Max number of bytes to read
** Return: Number of bytes read
*/
int net_read(char *buf, int nbytes);

/*
** Network driver interrupt handler
** rx/tx same
*/
void net_isr(int vector, int code);


/*
** 
*/
uint32_t ntohl(uint32_t n);
uint16_t ntohw(uint16_t n);
#endif
