#include "c_string.h"

int c_strcmp(char *a, char *b){
    for (int i = 0; a != '\0' && b != '\0'; i++){
        if (a[i] != b[i]){
            return a - b;
        }
    }
    if (a == '\0' && b == '\0') {
        return 0;
    } else if (a == '\0') {
        return 0-(int)b;
    }
    return (int)a;
}

unsigned int c_strlen(char *a){
    char *b = a;
    while(b){
        b++;
    }
    return (unsigned int)(b-a);
}
