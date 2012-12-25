#include <stdio.h>
#include <stdlib.h>
#include "message.pb-c.h"

int main (int argc, const char * argv[]) 
{
    Message msg = MESSAGE__INIT; // AMessage
    void *buf;                     // Buffer to store serialized data
    unsigned len;                  // Length of serialized data
        
        
    msg.np = 12;
    msg.npm = 0;
    msg.npa = 0;
    msg.load = 0.12;
    msg.uthresh = 1.14;
    msg.lthresh = 0.78;
    len = message__get_packed_size(&msg);
        
    buf = malloc(33);
    message__pack(&msg,buf);
        
    fprintf(stderr,"Writing %d serialized bytes\n",len); // See the length of message
    fwrite(buf,len,1,stdout); // Write to stdout to allow direct command line piping
        
    free(buf); // Free the allocated serialized buffer
    return 0;
}
