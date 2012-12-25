#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ALPHA 0.92
#define SEGMENT_SIZE 0x6400
#define SHM_ID_PATH "/tmp/migrate/shmid"
#define SEM_ID_PATH "/tmp/migrate/semid"
#define INFO_SHM_ID_PATH "/tmp/migrate/info_shmid"
#define INFO_SEM_ID_PATH "/tmp/migrate/info_semid"
#define COMMS_SHM_ID_PATH "/tmp/migrate/comms_shmid"
#define COMMS_SEM_ID_PATH "/tmp/migrate/comms_semid"
#define CALC_LOADAVG(loadvg,load) \
	loadvg += ALPHA * (load - loadavg);
#define SYS_LOAD "cat /proc/loadavg | awk 'n=1{print $n}'"

extern int no_of_peers;
extern int no_of_procs_migrated;
extern int no_of_procs_accepted;
extern double upper_thresh;
extern double lower_thresh;

void read_double_from_pipe (double *val,char *command)
{
	FILE *pipe;
	char *temp;
	temp = (char *)malloc(16);
	if(!temp) {
		fprintf(stderr,"Error: Could not allocate memory.\nExiting Daemon..");
		exit(1);
	}
	pipe = popen(command,"r");
	if(!pipe) {
		fprintf(stderr,"Error: Could not open pipe.\nExiting Daemon..");
		exit(1);
	}
	if(!fgets(temp,16,pipe)) {
		fprintf(stderr,"Error: Could not read string.\nExiting Daemon..");
		exit(1);
	}
	*val = atof(temp);
	if(pclose(pipe) == -1) {
		fprintf(stderr,"Error: Could not close pipe.\nExiting Daemon..");
		exit(1);
	}
	free(temp);	
}

union semun {
   int val;
   struct semid_ds *buf;
   unsigned short int *array;
   struct seminfo *__buf;
};

int binary_semaphore_wait (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = -1;
	operations[0].sem_flg = SEM_UNDO;
	
	return semop(semid,operations, 1);
}

int binary_semaphore_post (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = 1;
	operations[0].sem_flg = SEM_UNDO;
	
	return semop(semid, operations, 1);
}

void create_shm_seg(int *segment_id)
{
	*segment_id = shmget(IPC_PRIVATE, SEGMENT_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	if(*segment_id == -1) {
		fprintf(stderr,"Error: Cannot create shared memory segment.\nExiting Daemon..");
		exit(1);
	}
}

void create_bin_sem(int *sem_id)
{
	*sem_id = semget(IPC_PRIVATE,1,IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
	if(*sem_id == -1) {
		fprintf(stderr,"Error: Cannot create binary semaphore\nExiting Daemon..");
		exit(1);
	}
}

void write_int_to_file(char *path, int *val)
{
	FILE *file;
	
	file = fopen(path,"w");
	if(!file) {
		fprintf(stderr,"Error: Cannot open file to write integer.\nJust for the heck of it..Exiting Daemon..");
		exit(1);
	}
	fprintf(file,"%d\n",*val);
	if(fclose(file) != 0) {
		fprintf(stderr,"Error: Cannot close file to write integer.\nJust for the heck of it..Exiting Daemon..");
		exit(1);
	}
}

void attach_segment(char *sm, int *segment_id)
{
	sm = (char *) shmat(*segment_id, 0, 0);
	if(*sm == -1) {
		fprintf(stderr,"Error: Cannot attach segment.\nExiting Daemon..");
		exit(1);
	}
}

void init_binary_semaphore(int *sem_id, int val)
{
	union semun arg;
	unsigned short values[1];

	values[0] = 0;
	arg.array = values;
	semctl(*sem_id, 0, SETALL, arg);
}

void get_local_ip(char *string) {
	struct ifaddrs *ifAddrStruct = NULL;
	struct ifaddrs *ifa = NULL;
	void *tempAddrPtr = NULL;
	char *addressBuffer;
	addressBuffer = (char *) malloc(INET_ADDRSTRLEN);
	
	getifaddrs(&ifAddrStruct);

	for(ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if((ifa->ifa_addr->sa_family == AF_INET) && (strcmp(ifa->ifa_name,"lo") != 0)) {
			tempAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			inet_ntop(AF_INET, tempAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			strcpy(string,addressBuffer);
		}
	}
	free(addressBuffer);
	if(ifAddrStruct !=NULL)
		freeifaddrs(ifAddrStruct);
	
}
