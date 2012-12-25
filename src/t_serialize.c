/*
 * =====================================================================================
 *
 *       Filename:  t_serialize.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  Monday 06 June 2011 05:27:32  IST
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

int main()
{
	Commsinfo info = COMMSINFO__INIT;
	void *buf;
	unsigned len;

	info.ip = "192.168.1.1";
	info.load = 0.59;
	info.nop = 1;
	len = commsinfo__get_packed_size(&info);

	buf = malloc(len);
	commsinfo__pack(&info, buf);

	fprintf(stderr, "Writing %d serialized byted\n", len);
	fwrite(buf, len, 1, stdout);
	free(buf);

	return 0;
}
