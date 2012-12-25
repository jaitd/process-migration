/*
 * =====================================================================================
 *
 *       Filename:  update.c
 *
 *    Description:  test to get updated log
 *
 *        Version:  1.0
 *        Created:  Friday 03 June 2011 06:21:01  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>

int main() {
	char command_cp[] = "cp /var/log/load /tmp/migrate/load.temp";
	char command_df[] = "diff /tmp/migrate/load.temp /var/log/load | head -n 1";
	char output[256], *ptr, forint[16], *temp;
	char readline[1024];
	FILE *cmd;
	FILE *file;
	int i,line; 
	sprintf(output, "\0");

	while(1) {
	system(command_cp);
	cmd = popen(command_df,"r");
	fgets(output, 257, cmd);
	pclose(cmd);
	
	if(strcmp(output, "\0") == 0) {
		printf("no diff\n");
		continue;
	}

	ptr = strtok(output,"a");	
	printf("string: %s\n",ptr);

	line = atoi(ptr);

	printf("num: %d\n", line);

	i=0;
	file = fopen("/var/log/load","r");
	while(fgets(readline, 1025, file) != NULL) {
		if(i < line) {
			i++;
			continue;
		}
		else {
			printf("%s", readline);
			i++;
		}
	}
	fclose(file);	
	}
	return 0;
}
