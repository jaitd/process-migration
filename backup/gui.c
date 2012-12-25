#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "migrate.h"

WINDOW *title,*load,*load_daemon,*comms_daemon,*migrate_daemon,*extra_daemon;
char *shared_memory,*temp,*res;

void sig_handler (int signum)
{
	shmdt(shared_memory);
	free(temp);
	free(res);
	delwin(title);	
	delwin(load);
	endwin();
	exit(1);
}

void sigsegv_handler (int signum)
{
	fprintf(stderr,"Looks like, the load daemon is not running. Please start the load daemon for GUI to work correctly\n");
	exit(1);
}

void substr(char *res,char *string)
{
	int i;
	char *ptr;
	char *truncated;

	truncated = (char *) malloc(512);

	ptr = rindex(string,':');
	ptr++;
	for(i = 0; ptr[i] != '\0'; i++) {
		truncated[i] = ptr[i];
	}
	truncated[++i] = '\0';
	
	strcpy(res,truncated);
	free(truncated);
}


int main() {
	int height,width,pos_y,pos_x;
	int segment_id,sem_id,lines,i;
	FILE *file;
	char *string,*ip;
	struct timespec req,rem;
	req.tv_sec = 4;
	req.tv_nsec = 0;
	signal(SIGINT,sig_handler);
	signal(SIGTERM,sig_handler);
	signal(SIGSEGV,sigsegv_handler);
	
	string = (char *) malloc(32);
	ip = (char *) malloc(INET_ADDRSTRLEN);
	temp = (char *) malloc(1024);
	res = (char *) malloc(1024);
	
	file = fopen("/tmp/migrate/shmid","r");
	fgets(string,32,file);
	segment_id = atoi(string);
	fclose(file);
	
	file = fopen("/tmp/migrate/semid","r");
	fgets(string,32,file);
	sem_id = atoi(string);
	fclose(file);
	free(string);


	get_local_ip(ip);	
	
	shared_memory = (char *) shmat(segment_id,0,0);
	binary_semaphore_post(sem_id);

	initscr();
	cbreak();
	keypad(stdscr,TRUE);
	start_color();
	init_pair(1,COLOR_WHITE,COLOR_BLACK);
	init_pair(2,COLOR_YELLOW,COLOR_BLACK);
	init_pair(3,COLOR_CYAN,COLOR_BLACK);

	height = 3;
	width = 70;
	pos_y = (LINES - height) * 0.07;
	pos_x = (COLS - width) / 2;
	title = newwin(height,width,pos_y,pos_x);

	height = 7;
	width = 100;
	pos_y = (LINES - height) * 0.20;
	pos_x = (COLS - width) * 0.45;
	load = newwin(height, width,pos_y,pos_x);
	
	height = 12;
	width = 120;
	pos_y = (LINES - height) * 0.50;
	pos_x = (COLS - width) * 0.10;
	load_daemon = newwin(height, width,pos_y,pos_x);
	
	/*height = 12;
	width = 50;
	pos_y = (LINES - height) * 0.50;
	pos_x = (COLS - width) * 0.90;
	comms_daemon = newwin(height, width,pos_y,pos_x);*/

	height = 12;
	width = 50;
	pos_y = (LINES - height) * 0.90;
	pos_x = (COLS - width) * 0.10;
	migrate_daemon = newwin(height, width,pos_y,pos_x);
	
	height = 12;
	width = 50;
	pos_y = (LINES - height) * 0.90;
	pos_x = (COLS - width) * 0.90;
	extra_daemon = newwin(height, width,pos_y,pos_x);



	refresh();	
	box(title, 0, 0);
	box(load, 0, 0);
	box(load_daemon, 0, 0);
	//box(comms_daemon, 0, 0);
	box(migrate_daemon, 0, 0);
	box(extra_daemon, 0, 0);
	wbkgd(title,COLOR_PAIR(1));
	wbkgd(load,COLOR_PAIR(2));
	wbkgd(load_daemon,COLOR_PAIR(3));
	//wbkgd(comms_daemon,COLOR_PAIR(3));
	wbkgd(migrate_daemon,COLOR_PAIR(3));
	wbkgd(extra_daemon,COLOR_PAIR(3));
	idlok(load_daemon,TRUE);
	scrollok(load_daemon,TRUE);	

	mvwaddstr(title,1,10,"Process Migration - GUI for Statistics and Testing");
	wrefresh(title);
	wattron(load,A_BOLD);
	mvwaddstr(load,2,5,"Load: ");
	wattroff(load,A_BOLD);
	wattron(load,A_BOLD);
	mvwaddstr(load,2,17,"Local IP: ");
	wattroff(load,A_BOLD);
	mvwaddstr(load,2,27,ip);
	wattron(load,A_BOLD);
	mvwaddstr(load,2,42,"No. of Peers Connected: ");
	wattroff(load,A_BOLD);
	wattron(load,A_BOLD);
	mvwaddstr(load,2,70,"No. of Procs Migrated: ");
	wattroff(load,A_BOLD);
	wattron(load,A_BOLD);
	mvwaddstr(load,4,5,"No. of Procs Accepted: ");
	wattroff(load,A_BOLD);
	wattron(load,A_BOLD);
	mvwaddstr(load,4,42,"Upper Threshold: ");
	wattroff(load,A_BOLD);
	wattron(load,A_BOLD);
	mvwaddstr(load,4,65,"Lower Threshold: ");
	wattroff(load,A_BOLD);
	wattron(load_daemon,A_BOLD);
	mvwaddstr(load_daemon,0,2,"Load Daemon");
	wattroff(load_daemon,A_BOLD);
	wrefresh(load_daemon);
	wattron(comms_daemon,A_BOLD);
	mvwaddstr(comms_daemon,0,2,"Communications Daemon");
	wattroff(comms_daemon,A_BOLD);
	wrefresh(comms_daemon);
	wattron(migrate_daemon,A_BOLD);
	mvwaddstr(migrate_daemon,0,2,"Migration Control Daemon");
	wattroff(migrate_daemon,A_BOLD);
	wrefresh(migrate_daemon);
	wattron(extra_daemon,A_BOLD);
	mvwaddstr(extra_daemon,0,2,"Details");
	wattroff(extra_daemon,A_BOLD);
	wrefresh(extra_daemon);
	
	wrefresh(load);
	while(1) {
		binary_semaphore_wait(sem_id);	
		mvwaddstr(load,2,11,shared_memory);
		binary_semaphore_post(sem_id);
		wrefresh(load);		
		file = fopen("input","r");
		for(i = 0; fgets(temp,1025,file) != NULL; i++) {
			mvwaddstr(load_daemon,i+1,2,temp);
		}
		fclose(file);
		box(load_daemon, 0, 0);
		wrefresh(load_daemon);
		nanosleep(&req,&rem);
	}	
}
