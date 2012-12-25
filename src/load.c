#include <sys/types.>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "migrate.h"

int segment_id,sem_id;
char *shared_memory;

void sig_handler(int signum)
{
	if(signum == SIGINT) {
		syslog(LOG_CRIT,"This should be in the log");
	}
	else {
		syslog(LOG_INFO,"Received Interrupt");
		shmdt(shared_memory);
		shmctl(segment_id, IPC_RMID, NULL);
		semctl(sem_id, IPC_RMID, 0);
		syslog(LOG_INFO,"Cleanup Complete");
		syslog(LOG_INFO,"Load daemon terminated");
		closelog();
		exit(1);
	}
}

int main ()
{	
	pid_t pid,sid;
	int count;
	double load,loadavg = 0;

	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	
	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "Error: fork() failed\n");
		exit(EXIT_FAILURE);
	}
	if(pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);
	
	openlog("PM_Load Daemon:", LOG_CONS, LOG_LOCAL0);
	sid = setsid();
	if(sid < 0) {
		fprintf(stderr,"Error: Cannot create session\n");
		exit(EXIT_FAILURE);
	}

	if((chdir("/tmp/migrate/")) < 0) {
		fprintf(stderr,"Error: /tmp/migrate/ not found\n");
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	syslog(LOG_INFO,"Load daemon started");

	create_shm_seg(&segment_id);
	
	create_bin_sem(&sem_id);

	write_int_to_file(SHM_ID_PATH,&segment_id);
	write_int_to_file(SEM_ID_PATH,&sem_id);
	


	shared_memory = (char *) shmat(segment_id, 0, 0);
	init_binary_semaphore(&sem_id,0);
	syslog(LOG_INFO,"Migrate daemon has started");
	syslog(LOG_INFO,"IPC setup complete");
	syslog(LOG_INFO,"Load calculation and exchange started");
	while(1) {
		for(count = 0; count < 454; count++) {
			read_double_from_pipe(&load,SYS_LOAD);
			CALC_LOADAVG(loadavg,load);
		}
		binary_semaphore_wait(sem_id);
		sprintf(shared_memory,"%0.2f",loadavg);
		binary_semaphore_post(sem_id);
	}
	return 0;
}
