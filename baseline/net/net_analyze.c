#include "net_analyze.h"
#include "net.h"
#include "c_io.h"
#include "c_string.h"

netsig sigs[MAX_SIG];
uint32_t sig_count;
uint32_t sig_size;

char test_sigs[2][5] = { "AAAA", "BBBB" };

int net_analyze_init() {
    sig_count = 0;
#ifdef NET_ANALYZE_DEBUG
    c_printf("Net Analysis engine initializing\n");
#endif

    create_sig(test_sigs[0], 4, test_sigs[0], 4, test_sigs[0], 4);
    create_sig(test_sigs[1], 4, test_sigs[1], 4, test_sigs[1], 4);
    create_sig("CCCC",4,"CCCC",4,"CCCC",4);
    return -1;
}
/*
** #define RFD_COUNT               16
** #define RFD_DATA_SIZE           1496
** #define RFD_HEAD_SIZE           14 // 2x Mac + protocol
** typedef struct net_frame_s {
**     mac_addr mac_dst;
**     mac_addr mac_src;
**     uint16_t proto;
**     uint8_t data[RFD_DATA_SIZE];
** } __attribute__((__packed__)) netframe; 
**
*/

int create_sig(char *name, uint32_t namelen, char *desc, uint32_t desclen, char *sig, uint32_t siglen) {
    if (sig_count < MAX_SIG)
    {
        for (uint32_t i = 0; i < sig_count; i++)
        {
            if (c_strcmp(sigs[i].name, name) == 0)
            {
                sigs[i].desc = desc;
                sigs[i].sig = sig;
                sigs[i].desclen = desclen;
                sigs[i].siglen = siglen;
                #ifdef NET_ANALYZE_DEBUG
                c_printf("[create_sig] Updated Signature. Name: %s, Desc: %s, Sig: %s\n",name,desc,sig);
                #endif
                return 0;
            }
        }
        sigs[sig_count].name = name;
        sigs[sig_count].desc = desc;
        sigs[sig_count].sig = sig;
        sigs[sig_count].namelen = namelen;
        sigs[sig_count].desclen = desclen;
        sigs[sig_count].siglen = siglen;
        sig_count++;
        #ifdef NET_ANALYZE_DEBUG
        c_printf("[create_sig] Created Signature. Name: %s, Desc: %s, Sig: %s\n",name,desc,sig);
        #endif
        return 0;
    }
    return -1;
}

int delete_sig(char *name) {
    for (uint32_t i = 0; i < sig_count; i++)
    {
        if (c_strcmp(sigs[i].name,name) == 0)
        {
            sigs[i] = sigs[sig_count-1];
            #ifdef NET_ANALYZE_DEBUG
            c_printf("[delete_sig] Deleted Signature. Name: %s, Desc: %s, Sig: %s\n",sigs[i].name,sigs[i].desc,sigs[i].sig);
            #endif
            sig_count--;
            return 0;
        }
    }
    return -1;
}


int analyze_frame(netframe *frame) {
    for (uint16_t i = 0; i < RFD_DATA_SIZE; i++) {
        for (uint16_t j = 0; j < sig_count; j++) {
            for (uint16_t k = 0; k < sigs[j].siglen; k++) {
                if (frame->data[i] != sigs[j].sig[k])
                    break;

                if ( k == sigs[j].siglen-1 ) {
                    #ifdef NET_ANALYZE_DEBUG
                    c_printf("[analyse_frame] Signature Matched. Name: %s, Desc: %s, Sig: %s, Data: %s\n",sigs[i].name,sigs[i].desc,sigs[i].sig,frame->data);
                    #endif
                    return j;
                }
            }
        }
    }
    return -1;
}

int net_analyze_signature(netframe *frame) {
    /* check for signatures */
    return 0;
}

int net_analyze_protocol(netframe *frame) {
    /* check for protocol anomalies */
    return 0;
}
