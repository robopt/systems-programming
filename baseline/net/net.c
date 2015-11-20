#include "net.h"
#include "pci.h"
#include "x86arch.h"
#include "support.h"
#include "startup.h"
#include "klib.h"
#include "c_io.h"
#include "common.h"
#include "net_analyze.h"

rfd rx_buf[RFD_COUNT];
txd *tx_buf;

rfd *rx_start = NULL;
rfd *rx_end = NULL;
rfd *rx_cur = NULL;

net8255x *netdev;
mac_addr my_mac;

/*
** Initialize
** Return: 0 on Success, <0 on Error
*/
int _net_modinit() {
    
    // Fake MAC
    c_printf("Please input a value for the mac addr: ");
    uint8_t val = c_getchar();
    c_printf("\nMAC Address set to: ");
    for ( uint8_t i = 0; i < MAC_LEN; ++i) {
        my_mac.addr[i] = val;
        c_printf("%x",val);
        if (i < MAC_LEN - 1) {
            c_printf(":");
        }
    }
    c_printf("\n");

    // Vendor: 8086 - Intel Corporation
    // Device: 1229 - 82557, 82558, 82559
    // Class:  0x02 - Network Controller
    // SubCls: 0x00 - Ethernet Controller
    netdev->pci = find_dev(0x8086, 0x1229, 0x02, 0x00);
    if (netdev->pci == (void*)0) {
        // Device doesn't exist!
        return -1;
    }
#   ifdef _net_debug_
    c_printf("[net.c][net_init]: Intel 8255x @= %x, irq: %x\n", netdev->pci->address, netdev->pci->irq);
#   endif
    if (netdev->pci->bar0 &0x01) {
        netdev->scb = netdev->pci->bar0 & 0xFFFFFFFC;
#       ifdef _net_debug_
        c_printf("[net.c][net_init]: Bar0 has IO mapped bit. Bar0: %x SCB: %x\n", netdev->pci->bar0, netdev->scb);
#       endif
    }
    if (netdev->pci->bar1 &0x01) {
        netdev->scb = netdev->pci->bar1 & 0xFFFFFFFC;
#       ifdef _net_debug_
        c_printf("[net.c][net_init]: Bar1 has IO mapped bit. Bar1: %x SCB: %x\n", netdev->pci->bar1, netdev->scb);
#       endif
    }

#   ifdef _net_debug_
    c_printf("[net.c][net_init]: SCB Status: %x\n", __inw(netdev->scb + SCB_STATUS));
#   endif

    // Software Reset
    net_cmd_writel(SCB_PORT, 0);
    // Do not read/write to SCB registers for atleast ~1us, ~100ms to be safe.
    __delay(10);

    // Initialize
    net_cmd_writel(SCB_GEN_PTR, 0);
    
    // Setup Linear Addressing
    net_cmd_writeb(SCB_COMMAND, 0x60);// Load CU Base Address SCB Command
    net_cmd_writeb(SCB_COMMAND, 0x06);// Load RU Base Address SCB Command
    
    // Initialize RX Buffers
    rfd *prev = NULL;
    for (int i = 0; i < RFD_COUNT; ++i ) {
        //rfd *cur = (rfd *)(rfd_base_address + (i * sizeof(rfd)));
        rfd *cur = &rx_buf[i];
#       ifdef _net_debug_
        c_printf("[net.c][net_init]: Allocating: %x\n", cur);
#       endif
        _memset((uint8_t *)cur, sizeof(rfd), 0);
        cur->size = RFD_DATA_SIZE;
        if (rx_start == NULL){
            rx_start = cur;
        }
        if (prev != NULL) {
            prev->link_addr = (uint32_t)cur;
        }
        prev = cur;
    }
    // Make circular
    //prev->command = (SCB_CMD_EL | SCB_CMD_S);
    prev->link_addr = (uint32_t)rx_start;
    rx_cur = rx_start;
    rx_end = prev;
#   ifdef _net_debug_
    c_printf("[net.c][net_init]: rx_start: %x rx_start->link_addr: %x &rx_buf[1]: %x\n", rx_start, rx_start->link_addr, &rx_start[1]);
    c_printf("[net.c][net_init]: rx_start: %x rx_cur: %x rx_end: %x\n", rx_start, rx_cur, rx_end);
#   endif
    net_cmd_writel(SCB_GEN_PTR, (uint32_t)rx_start);
    net_cmd_writeb(SCB_COMMAND, SCB_RU_START);

#   ifdef _net_debug_
    /*
    ** General Status
    ** This is only available on 82559+ cards (Rev > 0x05).
    */
    if (netdev->pci->revision > 0x05) {
        uint8_t gstatus = net_cmd_readb(SCB_GEN_STATUS);
        c_printf("[net.c][net_init]: ");
        if ( gstatus & GEN_STATUS_LINK ) {
            c_printf("Link is up... ");
        } else {
            c_printf("Link is down... ");
        }
        if ( gstatus & GEN_STATUS_100 ) {
            c_printf("100Mbit... ");
        } else {
            c_printf("10Mbit... ");
        }

        if ( gstatus & GEN_STATUS_DUPLEX ) {
            c_printf("Full-Duplex.\n");
        } else {
            c_printf("Half-Duplex.\n");
        }
        c_printf("[net.c][net_init]: Rev: %x, GEN Status: %x\n", netdev->pci->revision, __inw(netdev->scb + SCB_STATUS), gstatus);
    } else {
        c_printf("[net.c][net_init]: Rev: %x, GEN Status not available\n", netdev->pci->revision, __inw(netdev->scb + SCB_STATUS));
    }

    uint8_t scb_status = net_cmd_readb(SCB_STATUS);
    uint8_t scb_statack = net_cmd_readb(SCB_STATACK) & STATACK_MASK;
    c_printf("[net.c][net_init]: SCB Status: %x, SCB STATACK: %x\n", scb_status, scb_statack);
#   endif
    
#   ifdef _net_debug_
    c_printf("[net.c][net_init]: Registering ISR on Vector: %x \n", netdev->pci->irq + 0x20);
#   endif
    //Register ISR
    __install_isr( netdev->pci->irq + 0x20, net_isr );
    return 0;
}


/*
** Write a byte cmd to SCB
*/
void net_cmd_writeb(uint8_t offset, uint8_t cmd){
#   ifdef _net_debug_
    c_printf("[net.c][net_cmd_writeb]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outb(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

/*
** Write a word cmd to SCB
*/
void net_cmd_writew(uint8_t offset, uint16_t cmd){
#   ifdef _net_debug_
    c_printf("[net.c][net_cmd_writew]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outw(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

/*
** Write a long cmd to SCB
*/
void net_cmd_writel(uint8_t offset, uint32_t cmd){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_writel]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outl(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

/*
** Read a command byte from SCB
*/
uint8_t net_cmd_readb(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readb]: Reading byte from %x.\n", netdev->scb+offset);
#   endif
    return ( __inb(netdev->scb + offset) );
}

/*
** Read a command word from SCB
*/
uint16_t net_cmd_readw(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readw]: Reading word from %x.\n", netdev->scb+offset);
#   endif
    return ( __inw(netdev->scb + offset) );
}

/*
** Read a command long from SCB
*/
uint32_t net_cmd_readl(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readl]: Reading long %x.\n", netdev->scb+offset);
#   endif
    return ( __inl(netdev->scb + offset) );
}

/*
** Wait for a command to finish.
*/
int net_cmd_wait(){
    uint32_t count = 0;
    uint8_t cmd;
    while ( (cmd = __inb(netdev->scb + SCB_COMMAND)) )
    {
#       ifdef _net_debug_
        c_printf("[net.c][net_cmd_wait]: SCB CMD: %x, Waiting on SCB CMD. %x\n", netdev->scb + SCB_COMMAND, cmd);
#       endif
        __delay(NET_CMD_DELAY);
        count++;
    }
#   ifdef _net_debug_
    c_printf("[net.c][net_cmd_wait]: Waited on SCB CMD for %d ticks (%d seconds).\n", count, NET_CMD_DELAY*count/10);
#   endif
    return count;
}

/*
** Used to test the communication of the driver.
** Reads in from console, sends to broadcast (FF:FF:FF:FF:FF:FF)
*/
void net_write_test(void){
    txd *tx = &tx_buf[0];
    tx->frame.mac_src = my_mac;
    tx->frame.proto = 0x08; // Because it's a protocol, not relevant
    tx->tx_buf_addr = 0;
    tx->command = SCB_CMD_EL | SCB_CMD_I | SCB_CMD_S | SCB_CMD_TRANS;
    c_printf("Enter a message to send: ");
    char test[64];
    c_gets(test,64);
    int i;
    for (i = 0; test[i] != '\0'; ++i) {
        tx_buf->frame.data[i] = test[i];
    }
    tx->tx_count = RFD_HEAD_SIZE + i;
#   ifdef _net_debug_
    c_printf("[net.c][net_write_test]: Sending frame to \"");
#   endif
    for (int i = 0; i < MAC_LEN; ++i){
        tx->frame.mac_dst.addr[i] = 0xFF;
#       ifdef _net_debug_
        c_printf("%x",tx->frame.mac_dst.addr[i]);
        if (i < MAC_LEN -1) {
            c_printf(":");
        }
#       endif
    }
#   ifdef _net_debug_
    c_printf("\" Data : ");
    for (int i = 0; i < tx->tx_count - RFD_HEAD_SIZE; ++i) {
        c_printf("%c",tx->frame.data[i]);
    }
#   endif
    net_cmd_writel(SCB_GEN_PTR, (uint32_t)tx);
    net_cmd_writeb(SCB_COMMAND, SCB_CU_START);
}

/*
** Write nbytes to the tx buffer
** Param [ buf ]: Buffer to write from
** Param [ nbytes ]: Number of bytes to write
** Return: Number of bytes written
*/
int net_write(mac_addr dst, char *buf, int nbytes) {
    //TODO: Implement circular buffer
    txd *tx = &tx_buf[0];
    tx->frame.mac_src = my_mac;
    tx->frame.mac_dst = dst;
    tx->frame.proto = 0x08;
    tx->tx_buf_addr = 0;
    tx->command = SCB_CMD_EL | SCB_CMD_I | SCB_CMD_S | SCB_CMD_TRANS;
    tx->tx_count = RFD_HEAD_SIZE + nbytes;
    for (int i = 0; i < nbytes; ++i){
        tx_buf->frame.data[i] = buf[i];
    }
#   ifdef _net_debug_
    c_printf("[net.c][net_write]: Sending frame to \"");
    for (int i = 0; i < MAC_LEN; ++i){
        c_printf("%x",tx->frame.mac_dst.addr[i]);
        if (i < MAC_LEN -1) {
            c_printf(":");
        }
    }
    c_printf("\" Data : ");
    for (int i = 0; i < nbytes; ++i) {
        c_printf("%c",buf[i]);
    }
#   endif

    // Send to NIC
    net_cmd_writel(SCB_GEN_PTR, (uint32_t)tx);
    net_cmd_writeb(SCB_COMMAND, SCB_CU_START);
    return nbytes;
}

/*
** Network driver interrupt handler
** rx/tx same
** ISSUE: stat/ack = 0x50, rus = 0x08
*/
void net_isr(int vector, int code){
    (void)code; //shut up compiler
#   ifdef _net_debug_
    c_printf("[net.c][net_isr] Interrupt. Vector: %d, Code: %d\n", vector, code);
#   endif
    uint8_t scb_status = net_cmd_readb(SCB_STATUS);
    uint8_t scb_statack = net_cmd_readb(SCB_STATACK) & STATACK_MASK;

    uint8_t scb_status_rus = (scb_status & SCB_RUS_MASK);
    uint8_t scb_status_cus = (scb_status & SCB_CUS_MASK);
    
    // SCB Stat/Ack
    if (scb_statack & STATACK_CU_READY ) {
        net_cmd_writeb(SCB_STATACK, STATACK_CU_READY);
    }
    if (scb_statack & STATACK_CU_LEAVE_ACT ) {
        net_cmd_writeb(SCB_STATACK, STATACK_CU_LEAVE_ACT);
    }
    if (scb_statack & STATACK_RU_LEAVE_RDY) {
        net_cmd_writeb(SCB_STATACK, STATACK_RU_LEAVE_RDY);
    }

    if (scb_statack & STATACK_RU_FRAME) {
#       ifdef _net_debug_
        c_printf("[net.c][net_isr] RU Frame Finished... cur->status: %x\n",rx_cur->status);
#       endif
        while (rx_cur->status != 0) {
            
            int ournet = 1;
            for (int i = 0; i < MAC_LEN; ++i){
                if (i < MAC_LEN - 1) {
                    if (rx_cur->frame.mac_src.addr[i] != rx_cur->frame.mac_src.addr[i+1])
                        ournet = 0;
                }
            }
#           ifdef _net_debug_
            c_printf("Src: ");
            for (int i = 0; i < MAC_LEN; ++i){
                c_printf("%x",rx_cur->frame.mac_src.addr[i]);
                if (i < MAC_LEN - 1) {
                    c_printf(":");
                    if (rx_cur->frame.mac_src.addr[i] != rx_cur->frame.mac_src.addr[i+1])
                        ournet = 0;
                }
            }
            c_printf(" Dst: ");
            for (int i = 0; i < MAC_LEN; ++i){
                c_printf("%x",(uint8_t)rx_cur->frame.mac_dst.addr[i]);
                if (i < MAC_LEN - 1) {
                    c_printf(":");
                }
            }
            c_printf(" Proto: %d, Size: %d, Written: %d", rx_cur->frame.proto, rx_cur->size, rx_cur->bytes_written & RFD_BYTE_WRITTEN_MASK);
#           endif
            if (ournet) {
                c_printf("WE GOT A MESSAGE: ");
                for (int i = 0; i < (rx_cur->bytes_written & RFD_BYTE_WRITTEN_MASK); ++i) {
                    c_printf("%c",rx_cur->frame.data[i]);
                }
                c_printf("\n");
                #ifdef _net_debug_
                c_printf("Analyzing packet for content.\n");
                #endif
                int result;
                if ((result = analyze_frame(&rx_cur->frame))){
                    c_printf("Signature matched! Signature #: %d\n",result);
                }
            }
            uint8_t done = 0;
            //if (cur->command & 0x8000) {
            if (rx_cur->command & SCB_CMD_EL) {
                done = 1;
            }
            
            rx_cur->status = 0;
            rx_cur->command = 0;
            rx_cur->size = RFD_DATA_SIZE;
            rx_cur = (rfd *)rx_cur->link_addr;
            if (done) { break; }
        }
        net_cmd_writeb(SCB_STATACK, STATACK_RU_FRAME);
    }

    // Software Interrupt
    if ( scb_statack & STATACK_SWI )
    {
#       ifdef _net_debug_
        c_printf("[net.c][net_isr] SCB Statack: SWI -> CU Finished.\n");
        net_cmd_writeb(SCB_STATACK, STATACK_SWI);
#       endif
    }

    // Flow Control Pause
    if ( scb_statack & STATACK_FLOW_PAUSE )
    {
#       ifdef _net_debug_
        c_printf("[net.c][net_isr] SCB Statack: STATACK_FLOW_PAUSE\n");
        net_cmd_writeb(SCB_STATACK, STATACK_FLOW_PAUSE);
#       endif
    }

    
    // SCB Status RU Status
    if (scb_status_rus & SCB_RUS_IDLE) { c_printf("[net.c][net_isr] SCB Status RUS: Idle.\n"); }
    if (scb_status_rus & SCB_RUS_SUSPEND) { c_printf("[net.c][net_isr] SCB Status RUS: Suspended.\n"); }
    if (scb_status_rus & SCB_RUS_NORESOURCE) {
        c_printf("[net.c][net_isr] SCB Status RUS: No Resources.\n");
        c_printf("[net.c][net_isr] RX Buffer @ CUR: Status: %x\n", rx_cur->status);
    }
#   ifdef _net_debug_
    if (scb_status_rus & SCB_RUS_READY) { c_printf("[net.c][net_isr] SCB Status RUS: Ready.\n"); }
#   endif

    // SCB Status CU Status
    if (scb_status_cus & SCB_CUS_IDLE) { c_printf("[net.c][net_isr] SCB Status CUS: Idle.\n"); }
    if (scb_status_cus & SCB_CUS_SUSPEND) { c_printf("[net.c][net_isr] SCB Status CUS: Suspended.\n"); }
    if (scb_status_cus & SCB_CUS_LPQ_ACTIVE) { c_printf("[net.c][net_isr] SCB Status CUS: LPQ Active.\n"); }
    if (scb_status_cus & SCB_CUS_HQP_ACTIVE) { c_printf("[net.c][net_isr] SCB Status CUS: HQP Active.\n"); }

#   ifdef _net_debug_
    c_printf("[net.c][net_isr] SCB_STATUS: %x\n", scb_status);
    c_printf("[net.c][net_isr] SCB_STATACK: %x\n", scb_statack);
#   endif

    // Acknowledge Interrupt
    __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
    if( vector >= 0x28 && vector <= 0x2f )
    {
        __outb( PIC_SLAVE_CMD_PORT, PIC_EOI );
    }
    //_kpanic("net", "ISR Triggered.");
    return; 
}
