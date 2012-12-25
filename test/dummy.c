#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	struct timespec req, rem;
	FILE *file;
	char string[128];

	req.tv_sec = 0;
	req.tv_nsec = 10000;

	srand((unsigned int) time(NULL));
	
	int i;
	for(;;) {
		printf("random no: %u\n",rand());
		
		if((rand() % 77) == 0) {
			file = popen("echo 9999^9999 | bc", "r");
			fgets(string, 129, file);
			fclose(file);
			printf("%s\n", string);
		}		
		
		file = fopen("/proc/loadavg","r");
		fgets(string, 129, file);
		fclose(file);
		printf("%s\n", string);	

		nanosleep(&req, &rem);
	}
	return 0;
}
