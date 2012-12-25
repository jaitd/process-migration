#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "list.h"
#include "migrate.h"
#include "info.pb-c.h"
#include "commsinfo.pb-c.h"
#define DMTCP_COORD_HOST "192.168.1.15"
#define DMTCP_COORD_PORT "7799"
#if (defined (__WINDOWS__))
#   define randof(num)  (int) ((float) (num) * rand () / (RAND_MAX + 1.0))
#else
#   define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))
#endif

pthread_t load, export, sighandler, mainthread, comms, update, decision;

typedef struct {
	int no_of_peers;
	int no_of_procs_migrated;
	int no_of_procs_accepted;
	double load;
	double upper_thresh;
	double lower_thresh;
} INFO;

struct cleanup_args {
	int shm_id;
	int sem_id;
	char *smem;
	void *buffer;
};
		
struct cleanup_args_comms {
	void *smem;
	void *buf;
};

typedef struct {
	struct list_head list;
	char ip[INET_ADDRSTRLEN];
	double load;
	int no_of_peers;
} node_list;	
		
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t llmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flagmutex = PTHREAD_MUTEX_INITIALIZER;
INFO this_node;
node_list mylist;
int thresh_flag = 0;


/**
 * list_size - returns no. of nodes in list.
 * @list: the &struct list_head pointer.
 */
int list_size(struct list_head *list)
{
	int count = 0;
	struct list_head *pos;
	list_for_each(pos, list) {
		count++;
	}
	return count;
}


/**
 * cmp - comparison function for list_sort.
 * @a: the first &struct list_head pointer.
 * @a: the second &struct list_head pointer.
 */
double cmp(struct list_head *a, struct list_head *b)
{
	node_list *x, *y;
	x = list_entry(a, node_list, list);
	y = list_entry(b, node_list, list);

	return x->load - y->load;
}

/**
 * list_sort - merge sort the list specified.
 * @head: the &struct list_head pointer.
 */
void list_sort(struct list_head *head)
{
        struct list_head *p, *q, *e, *list, *tail, *oldhead;
        int insize, nmerges, psize, qsize, i;

        list = head->next;
        list_del(head);
        insize = 1;
        for (;;) {
                p = oldhead = list;
                list = tail = NULL;
                nmerges = 0;

                while (p) {
                        nmerges++;
                        q = p;
                        psize = 0;
                        for (i = 0; i < insize; i++) {
                                psize++;
                                q = q->next == oldhead ? NULL : q->next;
                                if (!q)
                                        break;
                        }

                        qsize = insize;
                        while (psize > 0 || (qsize > 0 && q)) {
                                if (!psize) {
                                        e = q;
                                        q = q->next;
                                        qsize--;
                                        if (q == oldhead)
                                                q = NULL;
                                } else if (!qsize || !q) {
                                        e = p;
                                        p = p->next;
                                        psize--;
                                        if (p == oldhead)
                                                p = NULL;
                                } else if (cmp(p, q) <= 0) {
                                        e = p;
                                        p = p->next;
                                        psize--;
                                        if (p == oldhead)
                                                p = NULL;
                                } else {
                                        e = q;
                                        q = q->next;
                                        qsize--;
                                        if (q == oldhead)
                                                q = NULL;
                                }
                                if (tail)
                                        tail->next = e;
                                else
                                        list = e;
                                e->prev = tail;
                                tail = e;
                        }
                        p = q;
                }

                tail->next = list;
                list->prev = tail;

                if (nmerges <= 1)
                        break;

                insize *= 2;
        }

        head->next = list;
        head->prev = list->prev;
        list->prev->next = head;
        list->prev = head;
}

/**
 * list_min - minimum element of a sorted list.
 * @head: the &struct list_head pointer.
 */
double list_min(struct list_head *head) 
{
	node_list *tmp;
	
	tmp = list_entry(head->next, node_list, list);
	return tmp->load;	
}

/**
 * list_get_node - minimum load node of a sorted list.
 * @head: the &struct list_head pointer.
 * @ip: pointer to the ip string
 */
void list_get_node(struct list_head *head, char *ip)
{
	node_list *tmp;
	
	tmp = list_entry(head->next, node_list, list);
	strcpy(ip, tmp->ip);
}	

/**
 * list_max - maximum element of a sorted list.
 * @head: the &struct list_head pointer.
 */
double list_max(struct list_head *head) 
{
	node_list *tmp;
	
	tmp = list_entry(head->prev, node_list, list);
	return tmp->load;	
}

void checkpoint_all_procs() 
{
	FILE *file;
	char command[256];
	char string[16];

	sprintf(command, "dmtcp_command --host %s --port %s --quiet --bcheckpoint", DMTCP_COORD_HOST, DMTCP_COORD_PORT);
	file = popen(command,"r");
	fprintf(stderr,"Opened Pipe\n");
	fgets(string, 17, file);
	fprintf(stderr,"Read Pipe\n");
	pclose(file);
	fprintf(stderr,"Closed Pipe\n");

}

void get_proc_checkpoint_name(char *ckpt)
{
	
	if ((chdir("/tmp/migrate/checkpoints")) < 0) {
		fprintf(stderr,"Error: /tmp/migrate/checkpoints/ not found\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"Reached get_proc_checkpoint name function\n");
	char nckpt_command[] = "ls | grep ckpt | wc -l";
	int cnt, rnd = 0;
	FILE *pipe;
	char data[16], *file;
	char *getfn;
	
	file = (char *) malloc(128);
	getfn = (char *)malloc(512);
	srandom ((unsigned) time (NULL));
	pipe = popen(nckpt_command,"r");
	fgets(data, 33, pipe);
	cnt = atoi(data);
	pclose(pipe);
	
	if(cnt == 0) {
		strcpy(ckpt,"NULL");
		fprintf(stderr, "Returned null\n");
		return;
	}
	
	if(cnt == 1) {
		sprintf(getfn, "ls | grep ckpt");
		pipe = popen(getfn, "r");
		fgets(file, 129, pipe);
		pclose(pipe);
		
		free(getfn);	
		strcpy(ckpt, file);
		free(file);
	}
	else {
		while(rnd == 0) {
			rnd = randof(cnt+1);
		} 
		sprintf(getfn, "ls | grep ckpt | sed -n %dp",rnd);
		pipe = popen(getfn, "r");
		fgets(file, 129, pipe);
		pclose(pipe);
		
		free(getfn);	
		strcpy(ckpt, file);
		free(file);
	}
}
 
void cleanup_handler_load(void *arg) {
	char *mem = (char *)arg;
	shmdt(mem);
	syslog(LOG_INFO,"Load exchange thread cleanup complete");
}

void cleanup_handler(void *arg) {
	syslog(LOG_INFO,"Terminated main thread");
	closelog();
}

void cleanup_handler_export(void *arg) {
	struct cleanup_args *args = (struct cleanup_args *)arg;
	free(args->buffer);
	shmdt(args->smem);
	shmctl(args->shm_id, IPC_RMID, NULL);
	semctl(args->sem_id, IPC_RMID, 0);
	syslog(LOG_INFO,"Export details thread cleanup complete");
}


void cleanup_handler_comms(void *arg) {
	sleep(5);
	struct cleanup_args_comms *args = (struct cleanup_args_comms *)arg;
	shmdt(args->smem);
}

void *signal_handler(void *arg)
{
	sigset_t *set = (sigset_t *) arg;
	int s, sig;

	for(;;) {
		s = sigwait(set, &sig);
		if(s != 0) {
			fprintf(stderr,"Error: sigwait. Oh boy, you are in trouble\n");
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO,"Signal handler thread caugh signal %d\n",sig);
		break;
	}
	pthread_cancel(export);
	pthread_cancel(load);
	pthread_cancel(comms);
	pthread_cancel(update);
	pthread_cancel(mainthread);
}

void *get_load(void *arg)
{
	int segment_id,sem_id;
	FILE *file;
	char *string,*shared_memory;
	
	syslog(LOG_INFO,"Load exchange thread started");

	string = (char *) malloc(32);

	pthread_testcancel();
	file = fopen("/tmp/migrate/shmid","r");
	fgets(string,33,file);
	segment_id = atoi(string);
	fclose(file);

	pthread_testcancel();
	file = fopen("/tmp/migrate/semid","r");
	fgets(string,33,file);
	sem_id = atoi(string);
	fclose(file);

	free(string);
	shared_memory = (char *) shmat(segment_id, 0, 0);
	pthread_cleanup_push(&cleanup_handler_load, (void *)&shared_memory);
	binary_semaphore_post(sem_id);
	syslog(LOG_INFO,"Load exchange thread IPC setup complete");
	syslog(LOG_INFO,"Getting load information");
	for(;;) {
		pthread_testcancel();
		binary_semaphore_wait(sem_id);
		pthread_mutex_lock(&mutex1);
		this_node.load = atof(shared_memory);
		pthread_mutex_unlock(&mutex1);
		binary_semaphore_post(sem_id);
		pthread_testcancel();
		sleep(1);
	}
	pthread_cleanup_pop(0);
}

void *export_details(void *arg)
{
	Nodeinfo mynode = NODEINFO__INIT;
	struct cleanup_args args;
	void *buf;
	int segment_id, sem_id;
	char *shared_memory;

	syslog(LOG_INFO,"Export details thread started");
	
	buf = malloc(33);		

	create_shm_seg(&segment_id);
	create_bin_sem(&sem_id);

	write_int_to_file(INFO_SHM_ID_PATH, &segment_id);
	write_int_to_file(INFO_SEM_ID_PATH, &sem_id);

	shared_memory = (void *) shmat(segment_id, 0, 0);
	
	args.shm_id = segment_id;
	args.sem_id = sem_id;
	args.smem = shared_memory;
	args.buffer = buf;

	pthread_cleanup_push(&cleanup_handler_export,(void *) &args);
	init_binary_semaphore(&sem_id, 0);
	syslog(LOG_INFO,"Export details thread IPC setup complete");
	syslog(LOG_INFO,"Sending details to GUI");
	for(;;) {
		pthread_testcancel();
		pthread_mutex_lock(&mutex1);
		mynode.np = this_node.no_of_peers;
		mynode.npm = this_node.no_of_procs_migrated;
		mynode.npa = this_node.no_of_procs_accepted;
		mynode.load = this_node.load;
		mynode.uthresh = this_node.upper_thresh;
		mynode.lthresh = this_node.lower_thresh;
		pthread_mutex_unlock(&mutex1);
		pthread_testcancel();
		nodeinfo__pack(&mynode, buf);
		pthread_testcancel();
		binary_semaphore_wait(sem_id);
		memcpy(shared_memory, buf, 33);
		binary_semaphore_post(sem_id);
		pthread_testcancel();
		sleep(1);
	}
	pthread_cleanup_pop(0);
}

void *comms_fetch(void *args)
{
	printf("Comms fetch thread sleeping for comms to start\n");
	sleep(15);
	printf("Sleep over\n");

	pthread_testcancel();
	pthread_mutex_lock(&llmutex);
	INIT_LIST_HEAD(&mylist.list);
	pthread_mutex_unlock(&llmutex);
	
	int segment_id, sem_id;
	FILE *file;
	char string[32];
	void *shared_memory;
	void *buf;
	struct cleanup_args_comms arg;

	buf =  malloc (25);	
	file = fopen(COMMS_SHM_ID_PATH,"r");
	fgets(string,33,file);
	segment_id = atoi(string);
	fclose(file);

	file = fopen(COMMS_SEM_ID_PATH,"r");
	fgets(string,33,file);
	sem_id = atoi(string);
	fclose(file);

	pthread_testcancel();
	shared_memory = (void *) shmat(segment_id, 0, 0);

	arg.smem = shared_memory;
	arg.buf = buf;
	pthread_cleanup_push(&cleanup_handler_comms,(void *) &args);
	binary_semaphore_post(sem_id);
	printf("Waiting for COMMS to connect to other nodes\n");
	sleep(25);

	for(;;) {
		Commsinfo *info;
		node_list *tmp;
		struct list_head *pos;
		unsigned flag = 0;
		pthread_testcancel();
		binary_semaphore_wait(sem_id);
		memcpy(buf, shared_memory, 25);
		binary_semaphore_post(sem_id);
		
		info = commsinfo__unpack(NULL, 25, buf);
		pthread_mutex_lock(&llmutex);
		list_for_each(pos, &mylist.list) {
			tmp = list_entry(pos, node_list, list);
			if(strcmp(tmp->ip, info->ip) == 0) {
				tmp->load = info->load;
				tmp->no_of_peers = info->nop;
				printf("Updated node: %s\n", tmp->ip);
				flag = 1;
				break;
			}
		}
		if(flag == 0) {
			tmp = (node_list *) malloc(sizeof(node_list));
			strcpy(tmp->ip, info->ip);
			tmp->load = info->load;
			tmp->no_of_peers = info->nop;
			
			list_add(&(tmp->list), &(mylist.list));
			printf("Inserted into ll: ip: %s load: %0.2f nop: %d\n", info->ip, info->load, info->nop);
		}
		printf("Count: %d\n",list_size(&(mylist.list)));
		pthread_mutex_unlock(&llmutex);	
		commsinfo__free_unpacked(info, NULL);
		pthread_testcancel();
		sleep(1);
	}
	pthread_cleanup_pop(0);
}

void *update_routine(void *arg)
{
	node_list *tmp;
	struct list_head *pos;
	int nop, size;
	float min, max, avg, sum, uthresh, lthresh;
	sleep(30);
	while(1) {
		sum = 0;
		pthread_testcancel();
		pthread_mutex_lock(&llmutex);
			list_sort(&(mylist.list));
			nop = list_size(&(mylist.list));
			list_for_each(pos, &mylist.list) {
				tmp = list_entry(pos, node_list, list);
				sum += (tmp->load);
			}
			min = list_min(&(mylist.list));
			max = list_max(&(mylist.list));
		pthread_mutex_unlock(&llmutex);
		
		pthread_mutex_lock(&mutex1);
			if(this_node.load < min)
				min = this_node.load;
			if(this_node.load > max)
				max = this_node.load;
			avg = ((sum + this_node.load) / (nop + 1));
		pthread_mutex_unlock(&mutex1);

		uthresh = max - (max - ((max + avg) / 2));
		lthresh = (((min + avg) / 2) - min) - min;
		if(((uthresh-lthresh)/(max-min)) < 0.5) {
			pthread_mutex_lock(&flagmutex);
				thresh_flag = 0;
			pthread_mutex_unlock(&flagmutex);
		}
		if(((uthresh-lthresh)/(max-min)) > 0.5) {
			pthread_mutex_lock(&flagmutex);
				thresh_flag = 1;
			pthread_mutex_unlock(&flagmutex);
		}
	
		pthread_mutex_lock(&mutex1);
			this_node.no_of_peers = nop;
			this_node.upper_thresh = uthresh;
			this_node.lower_thresh = lthresh;
		pthread_mutex_unlock(&mutex1);
		pthread_testcancel();
		sleep(1);
	}
	
}


void *decision_routine(void *arg)
{	
	node_list *tmp;
	struct list_head *proc;
	FILE *info;
	double load, uthresh, iload, iuthresh;
	int nom;
	sleep(60);
	fprintf(stderr,"DECISION THREAD STARTED\n");
	while(1) {
		pthread_mutex_lock(&mutex1);
			uthresh = this_node.upper_thresh;
			load = this_node.load;
			nom = this_node.no_of_procs_migrated;
		pthread_mutex_unlock(&mutex1);
		if(nom > 2) {
			continue;
		}
		if(load > uthresh) { 
			fprintf(stderr,"1st Tier Sample -  LOAD IS GREATER THAN UTHRESH\n");
			pthread_mutex_lock(&flagmutex);
				if(thresh_flag == 0) {
					fprintf(stderr,"PROCESS(S) WILL NOT BE MIGRATED AS LOADS ARE CLUSTERED\n");
					pthread_mutex_unlock(&flagmutex);
					continue;
				}
				else if(thresh_flag == 1) {
					pthread_mutex_unlock(&flagmutex);
				}
			sleep(40);
			pthread_mutex_lock(&mutex1);
				iuthresh = this_node.upper_thresh;
				iload = this_node.load;
			pthread_mutex_unlock(&mutex1);
			if(iload > iuthresh) {
				fprintf(stderr,"2nd Tier Sample - LOAD IS GREATER THAN UTHRESH\n");
				pthread_mutex_lock(&mutex1); 				
				pthread_mutex_lock(&llmutex);
				//Locked mutexes so other threads cannot change them now
				checkpoint_all_procs();
				char *ckpt, *ptr, *cmd, *ip;
				char procid[10];
				int i = 0,j;
				ckpt = (char *) malloc(128);
				cmd = (char *) malloc(256);
				ip = (char *) malloc(24);
				get_proc_checkpoint_name(ckpt); // write this function using ~/Downloads/dmtcp/test/ckpt.c
				if(strcmp(ckpt,"NULL") == 0) {
					continue;
				}
				fprintf(stderr,"SELECTED PROCCESS FOR MIGRATION\n");
				ptr = index(ckpt, '-');
				ptr++;
				for(j=0; ptr[j] != '-'; j++) {
					procid[i] = ptr[j];
					i++;
				}
				procid[i++] = '\0';
				fprintf(stderr,"PROC ID:%s\n",procid);
				sprintf(cmd,"kill -TERM %s",procid);
				system(cmd);
				fprintf(stderr,"KILLED PROC. GOING FOR TRANSFER");
				list_get_node(&(mylist.list), ip);
				sprintf(cmd,"rrunbin %s %s", ip, ckpt);
				system(cmd); // write this function using ~/Downloads/dmtcp/test/ckpt.c
				sprintf(cmd,"rm %s",ckpt);
				system(cmd);
				free(ckpt);
				free(cmd);
				free(ip);
				this_node.no_of_procs_migrated++;
				pthread_mutex_unlock(&llmutex); 				
				pthread_mutex_unlock(&mutex1);
			}
		}
		sleep(2);
	}
}
int main() {

	sigset_t set;
	FILE *info;

	info = fopen("/tmp/migrate/extrainfo","w");
	fclose(info);

	openlog("PM_Migration Control Daemon:", LOG_CONS, LOG_LOCAL2);
	syslog(LOG_INFO, "Main thread started");
	
	mainthread = pthread_self();
	pthread_cleanup_push(&cleanup_handler, NULL);
	sigemptyset(&set);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);

	if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
		fprintf(stderr,"Error: pthread_sigmask\n");
		exit(EXIT_FAILURE);
	}

	this_node.no_of_peers = 0;
	this_node.no_of_procs_migrated = 0;
	this_node.no_of_procs_accepted = 0;
	this_node.load = 0.0;
	this_node.upper_thresh = 0.0;
	this_node.lower_thresh = 0.0;
	
	if ((chdir("/tmp/migrate/checkpoints")) < 0) {
		fprintf(stderr,"Error: /tmp/migrate/checkpoints/ not found\n");
		exit(EXIT_FAILURE);
	}

	pthread_create(&load, NULL, &get_load, NULL);
	pthread_create(&export, NULL, &export_details, NULL);
	pthread_create(&comms, NULL, &comms_fetch, NULL);
	pthread_create(&update, NULL, &update_routine, NULL);
	pthread_create(&sighandler, NULL, &signal_handler, (void *) &set);
	pthread_create(&decision, NULL, &decision_routine, NULL);
	
	while(1) {
		pthread_testcancel();
		sleep(2);
	}
	pthread_cleanup_pop(0);
	return 0;
}
