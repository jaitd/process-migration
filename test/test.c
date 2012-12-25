#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLINES 10

struct node 
{
	char string[4096];
	int loc;
	struct node *next;
};


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

int main(int argc, char *argv[])
{
	struct node *head,*tail,*temphead,*temp;
	int lines = 0,i;
	char my_string[4096];
	FILE *file;

	file = fopen(argv[1],"r");
	for(;fgets(my_string,4097,file)!=NULL;) {
		if(lines < MAXLINES) {
			struct node *temp;
			temp = (struct node *) malloc(sizeof(struct node));
			substr(temp->string,my_string);
			temp->loc = lines;
			temp->next = head;
			//wqprintf("string: %sloc: %d\n",temp->string,temp->loc);	
			if(head == NULL) {
				head = temp;
				tail = temp;
				temphead = temp;
			}else {
				tail->next = temp;
				tail = temp;
			}
			lines++;
		}
		if(lines == MAXLINES) {
			lines++;
			continue;
		}
		if(lines > MAXLINES) {
			substr(temphead->string,my_string);
			//printf("string: %sloc: %d\n",temphead->string,temphead->loc);	
			temphead = temphead -> next;
			lines++;
		}
	}
	lines = (lines > MAXLINES) ? MAXLINES : lines;
	for(i=0; i < lines; i++) {
		printf("%s",temphead->string);
		temphead = temphead->next;
	}

	temp = NULL;
	while(temp != tail) {
		temp = head;
		head = temp -> next;
		free(temp);
	}
	tail = NULL;

	return 0;
}

