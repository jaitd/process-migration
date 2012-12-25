/*
 * =====================================================================================
 *
 *       Filename:  deserialize.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  Thursday 2a6 May 2011 05:09:52  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "message.pb-c.h"
#define MAX_MSG_SIZE 1024

static size_t read_buffer (unsigned max_length, uint8_t *out)
{
	size_t cur_len = 0;
	uint8_t c,nread;
	while((nread = fread(out + cur_len, 1, max_length - cur_len, stdin)) != 0) {
	cur_len += nread;
	if(cur_len == max_length) {
		fprintf(stderr,"max message length exceeded\n");
		exit(1);
	}
	}
	return cur_len;
}

int main(int argc, char *argv[])
{
	Message *m;

	uint8_t buf[MAX_MSG_SIZE];
	size_t msg_len = read_buffer(MAX_MSG_SIZE, buf);
	m = message__unpack(NULL,msg_len, buf);
	if(m == NULL) {
		fprintf(stderr,"error unpacking incoming message\n");
		exit(1);
	}

	printf("Received: np=%d npm=%d npa=%d load=%f uthresh=%f lthresh=%f\n", m->np,m->npm,m->npa,m->load,m->uthresh,m->lthresh);

	message__free_unpacked(m, NULL);
	return 0;
}
