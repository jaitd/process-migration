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
#include <signal.h>
#include "info.pb-c.h"
#include "migrate.h"
#define MAX_MSG_SIZE 1024

int segment_id,sem_id;
char *shared_memory;

void sig_handler(int signum)
{
	shmdt(shared_memory);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	FILE *file;
	char *string;
	Nodeinfo *mynode;
	void *buf;
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	string = (char *) malloc(32);
	buf = malloc(33);

	file = fopen(INFO_SHM_ID_PATH,"r");
	fgets(string,33,file);
	segment_id = atoi(string);
	fclose(file);

	file = fopen(INFO_SEM_ID_PATH,"r");
	fgets(string,33,file);
	sem_id = atoi(string);
	fclose(file);

	free(string);
	shared_memory = (void *) shmat(segment_id, 0, 0);
	binary_semaphore_post(sem_id);
	for(;;) {
		binary_semaphore_wait(sem_id);
		memcpy(buf,shared_memory,33);
		binary_semaphore_post(sem_id);
		mynode = nodeinfo__unpack(NULL, 33, buf);
		if(mynode == NULL) {
			fprintf(stderr,"error unpacking incoming message\n");
			exit(1);
		}
		printf("Received: np=%d npm=%d npa=%d load=%f uthresh=%f lthresh=%f\n", mynode->np,mynode->npm,mynode->npa,mynode->load,mynode->uthresh,mynode->lthresh);

		nodeinfo__free_unpacked(mynode, NULL);
	}
	return 0;	
}
