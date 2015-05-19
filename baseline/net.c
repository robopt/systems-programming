#include "net.h"
#include "pci.h"
#include "x86arch.h"
#include "support.h"
#include "startup.h"
#include "klib.h"
#include "common.h"

#define _net_debug_
#ifdef _net_debug_
#include "c_io.h"
#endif

rfd rx_buf[RFD_COUNT];
txd tx_buf[TXD_COUNT];

rfd *rx_start;
rfd *rx_end;
rfd *rx_cur;

int rx_buf_index;
int rx_buf_cur;
int tx_buf_index;
int tx_buf_cur;

net8255x *netdev;

/*
** Initialize
** Return: 0 on Success, <0 on Error
*/
int _net_modinit() {
    rx_buf_index = 0;
    rx_buf_cur = 0;
    tx_buf_index = 0;
    tx_buf_cur = 0;
    // Vendor: 8086 - Intel Corporation
    // Device: 1229 - 82557, 82558, 82559
    // Class:  0x02 - Network Controller
    // SubCls: 0x00 - Ethernet Controller
    netdev->pci = find_dev(0x8086, 0x1229, 0x02, 0x00);
    if (netdev->pci == (void*)0) {
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
        rfd *cur = &(rx_buf[i]);
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
    prev->command = (SCB_CMD_EL | SCB_CMD_S);
    prev->link_addr = (uint32_t)rx_start;
    rx_cur = rx_start;
    rx_end = prev;
#   ifdef _net_debug_
    c_printf("[net.c][net_init]: rx_start: %x rx_start->link_addr: %x &rx_buf[1]: %x\n", rx_start, rx_start->link_addr, &rx_buf[1]);
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
        c_printf("[net.c][net_init]: Rev: %x\n", netdev->pci->revision, __inw(netdev->scb + SCB_STATUS));
    }

    uint8_t scb_status = net_cmd_readb(SCB_STATUS);
    uint8_t scb_statack = net_cmd_readb(SCB_STATACK) & STATACK_MASK;
    c_printf("[net.c][net_init]: SCB Status: %x, SCB STATACK: %x\n", scb_status, scb_statack);
    
#   endif
    
    //Register ISR
#   ifdef _net_debug_
    c_printf("[net.c][net_init]: Registering ISR on Vector: %x \n", netdev->pci->irq + 0x20);
#   endif
    __install_isr( netdev->pci->irq + 0x20, net_isr );
    //c_getchar();
    return 0;
}

void net_cmd_writeb(uint8_t offset, uint8_t cmd){
#   ifdef _net_debug_
    c_printf("[net.c][net_cmd_writeb]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outb(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

void net_cmd_writew(uint8_t offset, uint16_t cmd){
#   ifdef _net_debug_
    c_printf("[net.c][net_cmd_writew]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outb(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

void net_cmd_writel(uint8_t offset, uint32_t cmd){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_writel]: Writing %x to %x.\n", cmd, netdev->scb+offset);
#   endif
    __outb(netdev->scb + offset,cmd);
    if (offset == SCB_COMMAND) {
        net_cmd_wait();
    }
}

uint8_t net_cmd_readb(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readb]: Reading byte from %x.\n", netdev->scb+offset);
#   endif
    return ( __inb(netdev->scb + offset) );
}

uint16_t net_cmd_readw(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readw]: Reading word from %x.\n", netdev->scb+offset);
#   endif
    return ( __inw(netdev->scb + offset) );
}

uint32_t net_cmd_readl(uint8_t offset){
#   ifdef _net_debug_
        c_printf("[net.c][net_cmd_readl]: Reading long %x.\n", netdev->scb+offset);
#   endif
    return ( __inl(netdev->scb + offset) );
}

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
** Initialize the network card
** SW Reset and configuration
** Return: 0 on Success, <0 on Error
*/
int _net_modinit_driver() {
    return -1;
}

/*
** Write nbytes to the tx buffer
** Param [ buf ]: Buffer to write from
** Param [ nbytes ]: Number of bytes to write
** Return: Number of bytes written
*/
int net_write(char *buf, int nbytes) {

    //TODO: Implement circular buffer
    for ( int i = 0; i < nbytes; ++i ) {
        if (tx_buf_index >= 512) {
            tx_buf_index = 0;
        }
        //tx_buf[tx_buf_index] = buf[i];
    }
    return nbytes;
}

/*
** Read up to n bytes, place into buffer.
** Param [ buf ]: Buffer to write to
** Param [ nbytes ]: Max number of bytes to read
** Return: Number of bytes read
*/
int net_read(char *buf, int nbytes) {
    int read = 0;
    while (read < nbytes && tx_buf_cur != tx_buf_index) {
        //buf[read] = tx_buf[tx_buf_cur];
        if (tx_buf_cur >= 512) {
            tx_buf_cur = 0;
        } else {
            tx_buf_cur++;
        }
        read++;
    }
    return read;
}

/*
** Network driver interrupt handler
** rx/tx same
** ISSUE: stat/ack = 0x50, rus = 0x08
*/
void net_isr(int vector, int code){
#   ifdef _net_debug_
    c_printf("[net.c][net_isr] Interrupt. Vector: %d, Code: %d\n", vector, code);
    uint8_t scb_status = net_cmd_readb(SCB_STATUS);
    uint8_t scb_statack = net_cmd_readb(SCB_STATACK) & STATACK_MASK;

    uint8_t scb_status_rus = (scb_status & SCB_RUS_MASK);
    uint8_t scb_status_cus = (scb_status & SCB_CUS_MASK);
    
    // SCB Stat/Ack
    if (scb_statack & STATACK_RU_LEAVE_RDY) {
        net_cmd_writeb(SCB_STATACK, STATACK_RU_LEAVE_RDY);
    }

    if (scb_statack & STATACK_RU_FRAME) {
        c_printf("[net.c][net_isr] RU Frame Finished... cur->status: %x\n",rx_cur->status);
        while (rx_cur->status != 0) {
            c_printf("Got frame... \nData: %s\n",rx_cur->data);
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

    if ( scb_statack & STATACK_FLOW_PAUSE )
    {
        net_cmd_writeb(SCB_STATACK, STATACK_FLOW_PAUSE);
    }

    
    // SCB Status RU Status
    if (scb_status_rus & SCB_RUS_IDLE) { c_printf("[net.c][net_isr] SCB Status RUS: Idle.\n"); }
    if (scb_status_rus & SCB_RUS_SUSPEND) { c_printf("[net.c][net_isr] SCB Status RUS: Suspended.\n"); }
    if (scb_status_rus & SCB_RUS_NORESOURCE) {
        c_printf("[net.c][net_isr] SCB Status RUS: No Resources.\n");
        c_printf("[net.c][net_isr] RX Buffer @ CUR: Status: %x\n", rx_buf[rx_buf_cur].status);
    }
    if (scb_status_rus & SCB_RUS_READY) { c_printf("[net.c][net_isr] SCB Status RUS: Ready.\n"); }

    // SCB Status CU Status
    if (scb_status_cus & SCB_CUS_IDLE) { c_printf("[net.c][net_isr] SCB Status CUS: Idle.\n"); }
    if (scb_status_cus & SCB_CUS_SUSPEND) { c_printf("[net.c][net_isr] SCB Status CUS: Suspended.\n"); }
    if (scb_status_cus & SCB_CUS_LPQ_ACTIVE) { c_printf("[net.c][net_isr] SCB Status CUS: LPQ Active.\n"); }
    if (scb_status_cus & SCB_CUS_HQP_ACTIVE) { c_printf("[net.c][net_isr] SCB Status CUS: HQP Active.\n"); }

    c_printf("[net.c][net_isr] SCB_STATUS: %x\n", scb_status);
    c_printf("[net.c][net_isr] SCB_STATACK: %x\n", scb_statack);
#   endif

    __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
    if( vector >= 0x28 && vector <= 0x2f )
    {
        __outb( PIC_SLAVE_CMD_PORT, PIC_EOI );
    }
    //_kpanic("net", "ISR Triggered.");
    c_getchar();
    return; 
}
