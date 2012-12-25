#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "migrate.h"


struct node_list{
	struct list_head list;
	char  ip[INET_ADDRSTRLEN];
	double load;
	int no_of_peers;
};


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
	struct node_list *x, *y;
	x = list_entry(a, struct node_list, list);
	y = list_entry(b, struct node_list, list);

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

double list_min(struct list_head *head) 
{
	struct node_list *tmp;
	
	tmp = list_entry(head->next, struct node_list, list);
	return tmp->load;	
}

double list_max(struct list_head *head) 
{
	struct node_list *tmp;
	
	tmp = list_entry(head->prev, struct node_list, list);
	return tmp->load;	
}

int main(int argc, char **argv){

	struct node_list *tmp;
	struct list_head *pos, *q;
	unsigned int i;

	struct node_list mylist;
	INIT_LIST_HEAD(&mylist.list);
	for(i = 0; i < 3; i++) {
		tmp= (struct node_list *)malloc(sizeof(struct node_list));
		printf("Enter IP, load, nop\n");
		scanf("%s %lf %d", tmp->ip, &tmp->load, &tmp->no_of_peers);
		list_add(&(tmp->list), &(mylist.list));
	}
	printf("\n");

	printf("Printing the list:\n");
	list_for_each(pos, &mylist.list){
		 tmp= list_entry(pos, struct node_list, list);
		 printf("ip= %s load= %f nop= %d\n", tmp->ip, tmp->load, tmp->no_of_peers);
	}
	printf("\n");

	printf("\ncount : %d\n", list_size(&(mylist.list)));	
	
	list_sort(&(mylist.list));

	printf("min: %0.2lf\n", list_min(&(mylist.list)));
	tmp = list_entry(&(mylist.list), struct node_list, list);
	printf("max: %0.2lf\n", list_max(&(mylist.list)));

	printf("Printing the list:\n");
	list_for_each(pos, &mylist.list){
		 tmp= list_entry(pos, struct node_list, list);
		 printf("ip= %s load= %f nop= %d\n", tmp->ip, tmp->load, tmp->no_of_peers);
	}
	printf("\n");
	
	list_for_each_safe(pos, q, &mylist.list){
		 tmp= list_entry(pos, struct node_list, list);
		 list_del(pos);
		 free(tmp);
	}

	return 0;
}
