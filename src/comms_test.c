/*
 * =====================================================================================
 *
 *       Filename:  comms.c
 *
 *    Description:  Communcations Daemon
 *
 *        Version:  1.0
 *        Created:  Sunday 05 June 2011 11:08:45  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <zmq.h>
#include "migrate.h"
#include "commsinfo.pb-c.h"
#include "info.pb-c.h"

struct cleanup_args_pub {
	void *socket;
	void *buf;
	void *infobuf;
	void *smem;
};

struct cleanup_args_sub {
	int shm_id;
	int sem_id;
	void *smem;
	void *socket;
};

pthread_t publisher, subscriber, sighandler, mainthread;

void *signal_handler(void *arg) {
	sigset_t *set = (sigset_t *) arg;
	int s,sig;
	for(;;){
		s = sigwait(set, &sig);
		if(s != 0 ) {
			fprintf(stderr, "Error: sigwait");
			exit(EXIT_FAILURE);
		}
		break;
	}
	pthread_cancel(publisher);
	pthread_cancel(subscriber);
	pthread_cancel(mainthread);
}

void cleanup_handler_main(void *context) {
	pthread_join(publisher, NULL);
	pthread_join(subscriber, NULL);
	zmq_term(context);
	printf("Closed context. Exiting..\n");
}

void cleanup_handler_publisher(void *arg) {
	struct cleanup_args_pub *args = (struct cleanup_args_pub *)arg;
	shmdt(args->smem);
	free(args->buf);
	free(args->infobuf);
	zmq_close(args->socket);
	printf("Closed Publisher Socket\n");
}

void cleanup_handler_subscriber(void *arg) {
	struct cleanup_args_sub *args = (struct cleanup_args_sub *)arg;
	shmdt(args->smem);
	shmctl(args->shm_id, IPC_RMID, NULL);
	shmctl(args->sem_id, IPC_RMID, 0);
	zmq_close(socket);
	printf("Closed Subscriber Socket\n");
}


 
static void *publisher_routine(void *context) {
	void *pub = zmq_socket(context, ZMQ_PUB);
	Commsinfo info = COMMSINFO__INIT;
	Nodeinfo *mynode;	
	void *buf, *infobuf;
	char *ipaddr;
	char string[32];
	int segment_id,sem_id;
	FILE *file;
	void *shared_memory;
	struct cleanup_args_pub arg;

	buf = malloc (24);
	infobuf = malloc (33);
	ipaddr = (char *) malloc(INET_ADDRSTRLEN);
	get_local_ip(ipaddr);
	
	pthread_testcancel();
	file = fopen(INFO_SHM_ID_PATH,"r");
	fgets(string,33,file);
	segment_id = atoi(string);
	fclose(file);

	pthread_testcancel();
	file = fopen(INFO_SEM_ID_PATH,"r");
	fgets(string,33,file);
	sem_id = atoi(string);
	fclose(file);

	shared_memory = (void *) shmat(segment_id, 0, 0);

	arg.buf = buf;
	arg.infobuf = infobuf;
	arg.socket = pub;
	arg.smem = shared_memory;
	
	zmq_bind(pub, "tcp://*:5555");
	printf("Publisher Started\n");
	pthread_cleanup_push(&cleanup_handler_publisher, (void *) &arg);
	while(1) {
		info.ip = ipaddr;
		binary_semaphore_wait(sem_id);
		memcpy(infobuf, shared_memory, 33);
		binary_semaphore_post(sem_id);
		
		mynode = nodeinfo__unpack(NULL, 33, infobuf);
		if(mynode == NULL) {
			fprintf(stderr, "error unpacking message\n");
			exit(1);
		}
		info.load = mynode->load;
		info.nop = mynode->np;
		nodeinfo__free_unpacked(mynode, NULL);

		commsinfo__pack(&info, buf);
				
		pthread_testcancel();
		zmq_msg_t packet;
		zmq_msg_init_size (&packet, 25);
		memcpy(zmq_msg_data (&packet), buf, 25);
		zmq_send(pub, &packet, 0);
		zmq_msg_close(&packet);
		pthread_testcancel();
		sleep(1);
	}
	pthread_cleanup_pop(0);
}

static void *subscriber_routine(void *context) {
	void *sub = zmq_socket(context, ZMQ_SUB);
	sleep(2);
	zmq_connect(sub, "tcp://192.168.1.29:5555");
	zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);
	
	struct cleanup_args_sub args;
	int segment_id, sem_id;
	void *shared_memory;

	create_shm_seg(&segment_id);
	create_bin_sem(&sem_id);

	write_int_to_file(COMMS_SHM_ID_PATH, &segment_id);
	write_int_to_file(COMMS_SEM_ID_PATH, &sem_id);

	shared_memory = (void *) shmat(segment_id, 0, 0);
	
	args.shm_id = segment_id;
	args.sem_id = sem_id;
	args.smem = shared_memory;
	args.socket = sub;
	
	pthread_cleanup_push(&cleanup_handler_subscriber, (void *) &args);
	init_binary_semaphore(&sem_id, 0);
	printf("You have 20 seconds to start the publisher at the other end\n");
	sleep(20);
	while(1) {
		Commsinfo *info;
		zmq_msg_t packet;
		zmq_msg_init(&packet);
		zmq_recv(sub, &packet, 0);
		
		info = commsinfo__unpack(NULL, 25, zmq_msg_data(&packet));
		printf("Subscriber thread: ip: %s load: %0.2f nop: %d\n", info->ip, info->load, info->nop);
		commsinfo__free_unpacked(info, NULL);
		binary_semaphore_wait(sem_id);
		memcpy(shared_memory, zmq_msg_data(&packet), 25);
		binary_semaphore_post(sem_id);
		zmq_msg_close(&packet);
		pthread_testcancel();
		sleep(1);
	}
	pthread_cleanup_pop(0);
}
		

int main()
{
	void *context = zmq_init (1);
	sigset_t set;

	mainthread = pthread_self();
	pthread_cleanup_push(&cleanup_handler_main, context);
	sigemptyset(&set);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);

	if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
		fprintf(stderr,"Error: pthread_sigmask\n");
		exit(EXIT_FAILURE);
	}

	pthread_create(&publisher, NULL, publisher_routine, context);
	pthread_create(&subscriber, NULL, subscriber_routine, context);
	pthread_create(&sighandler, NULL, signal_handler, (void *) &set);

	while(1) {
		pthread_testcancel();
		sleep(2);
	}
	pthread_cleanup_pop(0);
	return 0;
}

