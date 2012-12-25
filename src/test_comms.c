#include "migrate.h"
#include "commsinfo.pb-c.h"

int main()
{
	int segment_id, sem_id;
	FILE *file;
	char string[32];
	char *shared_memory;
	void *buf;
	int i;

	buf = malloc (25);	
	file = fopen(COMMS_SHM_ID_PATH,"r");
	fgets(string,33,file);
	segment_id = atoi(string);
	fclose(file);

	file = fopen(COMMS_SEM_ID_PATH,"r");
	fgets(string,33,file);
	sem_id = atoi(string);
	fclose(file);

	shared_memory = (void *) shmat(segment_id, 0, 0);
	binary_semaphore_post(sem_id);

	for(i = 0; i < 100; i++) {
		Commsinfo *info;
		binary_semaphore_wait(sem_id);
		memcpy(buf, shared_memory, 25);
		binary_semaphore_post(sem_id);
		
		info = commsinfo__unpack(NULL, 25, buf);
		printf("Test prog: ip: %s load: %0.2f nop: %d\n", info->ip, info->load, info->nop);
		commsinfo__free_unpacked(info, NULL);
	}

	shmdt(shared_memory);
}


