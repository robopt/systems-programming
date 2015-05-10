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
 

/* Copyright from e100.h */
/*******************************************************************************
  Copyright(c) 1999 - 2004 Intel Corporation. All rights reserved.
  
  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the Free 
  Software Foundation; either version 2 of the License, or (at your option) 
  any later version.
  
  This program is distributed in the hope that it will be useful, but WITHOUT 
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
  more details.
  
  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59 
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
  
  Contact Information:
  Linux NICS <linux.nics@intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
*******************************************************************************/

#ifndef _net_h
#define _net_h
/* DEFINES TAKEN FROM e100.h subject to the above copyright */
/* Bit Mask definitions */
#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000
#define BIT_28      0x10000000

#define BIT_0_2     0x0007
#define BIT_0_3     0x000F
#define BIT_0_4     0x001F
#define BIT_0_5     0x003F
#define BIT_0_6     0x007F
#define BIT_0_7     0x00FF
#define BIT_0_8     0x01FF
#define BIT_0_13    0x3FFF
#define BIT_0_15    0xFFFF
#define BIT_1_2     0x0006
#define BIT_1_3     0x000E
#define BIT_2_5     0x003C
#define BIT_3_4     0x0018
#define BIT_4_5     0x0030
#define BIT_4_6     0x0070
#define BIT_4_7     0x00F0
#define BIT_5_7     0x00E0
#define BIT_5_12    0x1FE0
#define BIT_5_15    0xFFE0
#define BIT_6_7     0x00c0
#define BIT_7_11    0x0F80
#define BIT_8_10    0x0700
#define BIT_9_13    0x3E00
#define BIT_12_15   0xF000
#define BIT_8_15    0xFF00

#define BIT_16_20   0x001F0000
#define BIT_21_25   0x03E00000
#define BIT_26_27   0x0C000000
/* Interrupt status/ack fields */
/* ER and FCP interrupts for 82558 masks  */
#define SCB_STATUS_ACK_MASK        BIT_8_15     /* Status Mask */
#define SCB_STATUS_ACK_CX          BIT_15       /* CU Completed Action Cmd */
#define SCB_STATUS_ACK_FR          BIT_14       /* RU Received A Frame */
#define SCB_STATUS_ACK_CNA         BIT_13       /* CU Became Inactive (IDLE) */
#define SCB_STATUS_ACK_RNR         BIT_12       /* RU Became Not Ready */
#define SCB_STATUS_ACK_MDI         BIT_11       /* MDI read or write done */
#define SCB_STATUS_ACK_SWI         BIT_10       /* S/W generated interrupt */
#define SCB_STATUS_ACK_ER          BIT_9        /* Early Receive */
#define SCB_STATUS_ACK_FCP         BIT_8        /* Flow Control Pause */

/*- CUS Fields */
#define SCB_CUS_MASK            (BIT_6 | BIT_7) /* CUS 2-bit Mask */
#define SCB_CUS_IDLE            0       /* CU Idle */
#define SCB_CUS_SUSPEND         BIT_6   /* CU Suspended */
#define SCB_CUS_ACTIVE          BIT_7   /* CU Active */

/*- RUS Fields */
#define SCB_RUS_IDLE            0       /* RU Idle */
#define SCB_RUS_MASK            BIT_2_5 /* RUS 3-bit Mask */
#define SCB_RUS_SUSPEND         BIT_2   /* RU Suspended */
#define SCB_RUS_NO_RESOURCES    BIT_3   /* RU Out Of Resources */
#define SCB_RUS_READY           BIT_4   /* RU Ready */
#define SCB_RUS_SUSP_NO_RBDS    (BIT_2 | BIT_5) /* RU No More RBDs */
#define SCB_RUS_NO_RBDS         (BIT_3 | BIT_5) /* RU No More RBDs */
#define SCB_RUS_READY_NO_RBDS   (BIT_4 | BIT_5) /* RU Ready, No RBDs */

/* SCB Command Word bit definitions */
/*- CUC fields */
/* Changing mask to 4 bits */
#define SCB_CUC_MASK            BIT_4_7 /* CUC 4-bit Mask */
#define SCB_CUC_NOOP            0
#define SCB_CUC_START           BIT_4   /* CU Start */
#define SCB_CUC_RESUME          BIT_5   /* CU Resume */
#define SCB_CUC_UNKNOWN         BIT_7   /* CU unknown command */
/* Changed for 82558 enhancements */
#define SCB_CUC_STATIC_RESUME   (BIT_5 | BIT_7) /* 82558/9 Static Resume */
#define SCB_CUC_DUMP_ADDR       BIT_6   /* CU Dump Counters Address */
#define SCB_CUC_DUMP_STAT       (BIT_4 | BIT_6) /* CU Dump stat. counters */
#define SCB_CUC_LOAD_BASE       (BIT_5 | BIT_6) /* Load the CU base */
/* Below was defined as BIT_4_7 */
#define SCB_CUC_DUMP_RST_STAT   BIT_4_6 /* CU Dump & reset statistics cntrs */

/*- RUC fields */
#define SCB_RUC_MASK            BIT_0_2 /* RUC 3-bit Mask */
#define SCB_RUC_START           BIT_0   /* RU Start */
#define SCB_RUC_RESUME          BIT_1   /* RU Resume */
#define SCB_RUC_ABORT           BIT_2   /* RU Abort */
#define SCB_RUC_LOAD_HDS        (BIT_0 | BIT_2) /* Load RFD Header Data Size */
#define SCB_RUC_LOAD_BASE       (BIT_1 | BIT_2) /* Load the RU base */
#define SCB_RUC_RBD_RESUME      BIT_0_2 /* RBD resume */

/* Interrupt fields (assuming byte addressing) */
#define SCB_INT_MASK            BIT_0   /* Mask interrupts */
#define SCB_SOFT_INT            BIT_1   /* Generate a S/W interrupt */
/*  Specific Interrupt Mask Bits (upper byte of SCB Command word) */
#define SCB_FCP_INT_MASK        BIT_2   /* Flow Control Pause */
#define SCB_ER_INT_MASK         BIT_3   /* Early Receive */
#define SCB_RNR_INT_MASK        BIT_4   /* RU Not Ready */
#define SCB_CNA_INT_MASK        BIT_5   /* CU Not Active */
#define SCB_FR_INT_MASK         BIT_6   /* Frame Received */
#define SCB_CX_INT_MASK         BIT_7   /* CU eXecution w/ I-bit done */
#define SCB_BACHELOR_INT_MASK   BIT_2_7 /* 82558 interrupt mask bits */

#define SCB_GCR2_EEPROM_ACCESS_SEMAPHORE BIT_7

/* EEPROM bit definitions */
/*- EEPROM control register bits */
#define EEPROM_FLAG_ASF  0x8000
#define EEPROM_FLAG_GCL  0x4000

#define EN_TRNF          0x10   /* Enable turnoff */
#define EEDO             0x08   /* EEPROM data out */
#define EEDI             0x04   /* EEPROM data in (set for writing data) */
#define EECS             0x02   /* EEPROM chip select (1=hi, 0=lo) */
#define EESK             0x01   /* EEPROM shift clock (1=hi, 0=lo) */

/*- EEPROM opcodes */
#define EEPROM_READ_OPCODE          06
#define EEPROM_WRITE_OPCODE         05
#define EEPROM_ERASE_OPCODE         07
#define EEPROM_EWEN_OPCODE          19  /* Erase/write enable */
#define EEPROM_EWDS_OPCODE          16  /* Erase/write disable */

/*- EEPROM data locations */
#define EEPROM_NODE_ADDRESS_BYTE_0      0
#define EEPROM_COMPATIBILITY_WORD       3
#define EEPROM_PWA_NO                   8
#define EEPROM_ID_WORD                  0x0A
#define EEPROM_CONFIG_ASF               0x0D
#define EEPROM_SMBUS_ADDR               0x90

#define EEPROM_SUM                      0xbaba



/* Self Test Results*/
#define CB_SELFTEST_FAIL_BIT        BIT_12
#define CB_SELFTEST_DIAG_BIT        BIT_5
#define CB_SELFTEST_REGISTER_BIT    BIT_3
#define CB_SELFTEST_ROM_BIT         BIT_2

#define CB_SELFTEST_ERROR_MASK ( \
                 CB_SELFTEST_FAIL_BIT | CB_SELFTEST_DIAG_BIT | \
                 CB_SELFTEST_REGISTER_BIT | CB_SELFTEST_ROM_BIT)


/* E100 Action Commands */
#define CB_IA_ADDRESS           1
#define CB_CONFIGURE            2
#define CB_MULTICAST            3
#define CB_TRANSMIT             4
#define CB_LOAD_MICROCODE       5
#define CB_LOAD_FILTER          8
#define CB_MAX_NONTX_CMD        9
#define CB_IPCB_TRANSMIT        9

/* Pre-defined Filter Bits */
#define CB_FILTER_EL            0x80000000
#define CB_FILTER_FIX           0x40000000
#define CB_FILTER_ARP           0x08000000
#define CB_FILTER_IA_MATCH      0x02000000

/* Command Block (CB) Field Definitions */
/*- CB Command Word */
#define CB_EL_BIT           BIT_15      /* CB EL Bit */
#define CB_S_BIT            BIT_14      /* CB Suspend Bit */
#define CB_I_BIT            BIT_13      /* CB Interrupt Bit */
#define CB_TX_SF_BIT        BIT_3       /* TX CB Flexible Mode */
#define CB_CMD_MASK         BIT_0_3     /* CB 4-bit CMD Mask */
#define CB_CID_DEFAULT      (0x1f << 8) /* CB 5-bit CID (max value) */

/*- CB Status Word */
#define CB_STATUS_MASK          BIT_12_15       /* CB Status Mask (4-bits) */
#define CB_STATUS_COMPLETE      BIT_15  /* CB Complete Bit */
#define CB_STATUS_OK            BIT_13  /* CB OK Bit */
#define CB_STATUS_VLAN          BIT_12 /* CB Valn detected Bit */
#define CB_STATUS_FAIL          BIT_11  /* CB Fail (F) Bit */

/*misc command bits */
#define CB_TX_EOF_BIT           BIT_15  /* TX CB/TBD EOF Bit */

/* Config params */
#define CB_CFIG_BYTE_COUNT          22  /* 22 config bytes */
#define CB_CFIG_D102_BYTE_COUNT    10



/*typedef struct CSR_struct {
    short StatusWord;   // SCB Command Word
    short CmdWord;      // SCB Status Word
    int GeneralPtr;     // SCB General Pointer
    int Port;           // Port Interface
    short RESERVED;     // RESERVED - DO NOT USE
    short EECtlReg;     // EEPROM Control Register
    int MDICtlReg;      // MDI Control Register
    int RXDMACount;     // RX DMA Byte Count
    char RESERVED2;     // RESERVED - DO NOT USE
    int FlowCtlReg;     // Flow Control Register
    char PMDR;          // Power Management Driver Register
    char GeneralCtl;    // General Control
    char GeneralSts;    // General STatus
    short RESERVED3;    // RESERVED - DO NOT USE
    int RESERVED4;      // RESERVED - DO NOT USE
    int FuncEvtReg;     // Function Event Register
    int FuncEvtMskReg;  // Function Event Mask Register
    int FuncPrsStateReg;// Function Present State Register
    int ForceEvtReg;    // Force Event Register
} CSR;

typedef struct SCBStatus_struct {
    int :2 zero;        // Bits 0-1: 0
    int :3 RUS;         // Bits 2-5: RUS
    int :2 CUS;         // Bits 6-7: CUS
    int :1 FCP;         // Bit 8 Flow Control Pause
    int :1 RES;         // Bit 9 Reserved
    int :1 SWI;         // Bit 10 Software Interrupts
    int :1 MDI;         // Bit 11 MDI 
    int :1 RNR;         // Bit 12 Left RU Ready
    int :1 CNA;         // Bit 13 Left CU Active
    int :1 RNR;         // Bit 14 RU Frame Finish
    int :1 CXTNO;       // Bit 15 CU Command Finish
} SCBStatusWord;

typedef struct SCB_struct {
    short StatusWord;   // SCB Command Word
    SCBStatusWord StatusWord;
    short CmdWord;      // SCB Status Word
    int GeneralPtr;     // SCB General Pointer
} SCB;


typedef struct MDIControl_struct {
    
}

enum SCBStatusRUS {
    idle = 0;
    suspended = 1;
    noresource = 2;
    reserve1 = 3;
    ready = 4;
    reserve2 = 5;
    reserve3 = 6;
    reserve4 = 7;
    reserve5 = 8;
    reserve6 = 9;
    reserve7 = 10;
    reserve8 = 11;
    reserve9 = 12;
    reserve10 = 13;
    reserve11 = 14;
    reserve12 = 15;
}

enum SCBStatusCUS {
    idle = 0;
    suspended = 1;
    lpqactive = 2;
    hqpactive = 3;
}
*/
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

/*
** Write nbytes to the tx buffer
** Param [ buf ]: Buffer to write from
** Param [ nbytes ]: Number of bytes to write
** Return: Number of bytes written
*/
int net_write(void *buf, int nbytes);

/*
** Read up to n bytes, place into buffer.
** Param [ buf ]: Buffer to write to
** Param [ nbytes ]: Max number of bytes to read
** Return: Number of bytes read
*/
int net_read(void *buf, int nbytes);

/*
** Network driver interrupt handler
** rx/tx same
*/
void net_isr(int vector, int code);

void net_isr_tx(void);

void net_isr_rx(void);
#endif
