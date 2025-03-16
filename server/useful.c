#include "useful.h"

int usef_CharToInt(char *in)
{
    int out = 0;
    char c = *in;
    for(int i=0; c!=0; i++,c=in[i]) {
        out *= 10;
        out += c - '0';
    }

    return out;
}