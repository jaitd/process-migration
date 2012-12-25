/*
 * =====================================================================================
 *
 *       Filename:  t_deserialize.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  Monday 06 June 2011 07:15:48  IST
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
#include "commsinfo.pb-c.h"
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
	Commsinfo *m;

	uint8_t buf[MAX_MSG_SIZE];
	size_t msg_len = read_buffer(MAX_MSG_SIZE, buf);
	m = commsinfo__unpack(NULL,msg_len, buf);
	if(m == NULL) {
		fprintf(stderr,"error unpacking incoming message\n");
		exit(1);
	}

	printf("Received: ip=%s load=%0.2f nop=%d\n", m->ip,m->load,m->nop);

	commsinfo__free_unpacked(m, NULL);
	return 0;
}
