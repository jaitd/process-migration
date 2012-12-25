/*
 * =====================================================================================
 *
 *       Filename:  cdk.c
 *
 *    Description:  testing
 *
 *        Version:  1.0
 *        Created:  Thursday 26 May 2011 12:48:32  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include "cdk.h"

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

int main()
{
	CDKSCREEN *cdkscreen = (CDKSCREEN *)NULL;
	CDKSWINDOW *loadDaemon = (CDKSWINDOW *)NULL;
	CDKSWINDOW *commsDaemon = (CDKSWINDOW *)NULL;
	CDKSWINDOW *migrateDaemon = (CDKSWINDOW *)NULL;
	CDKSWINDOW *extraInfo = (CDKSWINDOW *)NULL;
	CDKLABEL *title = (CDKLABEL *)NULL;
	
	WINDOW *cursesWin = (WINDOW *)NULL;
	char string[1024],temp[1024];
	char *mesg[4];
	FILE *file;

	cursesWin = initscr();
	cdkscreen = initCDKScreen(cursesWin);

	initCDKColor();
	sprintf(temp,"<C></B/3>Process Migration - GUI for Statistics and Testing");
	mesg[0] = copyChar(temp);
	title = newCDKLabel(cdkscreen, CENTER, TOP, mesg,1,1,0);
      	setCDKLabel (title, mesg, 1, 1);
      	drawCDKLabel (title, 1);

	loadDaemon = newCDKSwindow (cdkscreen, (COLS - 50) * 0.10, (LINES - 12) * 0.40, 12, 50, "<C></B/5>Load Daemon", 1000, 1, 0);
	commsDaemon = newCDKSwindow (cdkscreen, (COLS - 50) * 0.90, (LINES - 12) * 0.40, 12, 50, "<C></B/5>Commucation Daemon", 1000, 1, 0);
	migrateDaemon = newCDKSwindow (cdkscreen, (COLS - 50) * 0.10, (LINES - 12) * 0.90, 12, 50, "<C></B/5>Migration Control Daemon", 1000, 1, 0);
	extraInfo = newCDKSwindow (cdkscreen, (COLS - 50) * 0.90, (LINES - 12) * 0.90, 12, 50, "<C></B/5>Information", 1000, 1, 0);

	//setCDKSwindowBackgroundColor (commandOutput, "</5>");
	refreshCDKScreen(cdkscreen);

	file = fopen("/var/log/load","r");
	while(fgets(string, 1025, file) != NULL) {
		substr(&temp[0],&string[0]);
		addCDKSwindow (loadDaemon, temp, BOTTOM);
	}
	fclose(file);	
	sleep(10);	
	freeChar (mesg[0]);
	destroyCDKSwindow(loadDaemon);
	destroyCDKSwindow(commsDaemon);
	destroyCDKSwindow(migrateDaemon);
	destroyCDKSwindow(extraInfo);
	delwin(cursesWin);
	endCDK();
	exit(0);

}
