#include "net.h"
#include "pci.h"
#include "x86arch.h"

#define _net_debug_
#ifdef _net_debug_
#include "c_io.h"
#endif

char rx_buf[512];
char tx_buf[512];

int rx_buf_index;
int rx_buf_cur;
int tx_buf_index;
int tx_buf_cur;


/*
** Initialize
** Return: 0 on Success, <0 on Error
*/
int _net_modinit() {
    // Vendor: 8086 - Intel Corporation
    // Device: 1019 - 82547EI Gigabit Ethernet Controller
    // Device: 1229 - 82557, 82558, 82559
    // Class:  0x02 - Network Controller
    // SubCls: 0x00 - Ethernet Controller
    uint32_t dev_addr = find_dev((uint16_t)8086, (uint16_t)1019, (uint8_t)0x02, (uint8_t)0x00);
#   ifdef _net_debug_
    c_puts("[net.c][net_init]: find_dev(8086, 1019, 0x02, 0x00) = ");
    c_printf("%d \n", dev_addr);
#   endif
    if ( !dev_addr ) {
        dev_addr = find_dev(8086,1229, 0x02, 0x00);
#       ifdef _net_debug_
        c_puts("[net.c][net_init]: find_dev(8086, 1229, 0x02, 0x00) = ");
        c_printf("%d \n", dev_addr);
#       endif
    }

    __install_isr( INT_VEC_NETWORK, net_isr);
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
int net_write(void *buf, int nbytes) {
    return -1;
}

/*
** Read up to n bytes, place into buffer.
** Param [ buf ]: Buffer to write to
** Param [ nbytes ]: Max number of bytes to read
** Return: Number of bytes read
*/
int net_read(void *buf, int nbytes) {
    return -1;
}

/*
** Network driver interrupt handler
** rx/tx same
*/
void net_isr(int vector, int code){
    return; 
}

/*
** Network interrupt TX handler
*/
void net_isr_tx(){
    return;
}

/*
** Network interrupt RX handler
*/
void net_isr_rx(){
    return;
}
