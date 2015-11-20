#include "net.h"
#define DEFAULT_SIG_SIZE 512
#define MAX_SIG 512

typedef struct netsig_struct {
    char *name;
    char *desc;
    char *sig;
    unsigned int namelen;
    unsigned int desclen;
    unsigned int siglen;
} netsig;

int net_analyze_init(void);
int create_sig(char *name, uint32_t namelen, char *desc, uint32_t desclen, char *sig, uint32_t siglen);
int delete_sig(char *name);
int analyze_frame(netframe *frame);
int net_analyze_signature(netframe *frame);
int net_analyze_protocol(netframe *frame);
