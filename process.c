#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>  
#include <sys/sem.h>

#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/msg.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/shm.h>

#define MAX_BUFFER_SIZE 100 // Maximum buffer size
#define MAX_PAGES 1000 // Maximum pages
#define TOSCH 10 // TOSCH+id will be used to read msg from sch
#define FROMSCH 20  // FROMSCH+id will be used to read msg from sch
#define TOMMU 10 // TOMMU+id will be used to read msg from MMU
#define FROMMMU 20 // FROMMMU+id will be used to read msg from MMU

int pg_no[MAX_PAGES] ;
int no_of_pages;

typedef struct mmumsgbuf_send {
	long    mtype;  // message type       
	int id;
	int pageno;
} mmumsgbuf_send;

typedef struct mmumsgbuf_recv {
	long    mtype;  // 	message type   
	int frameno;
} mmumsgbuf_recv;

typedef struct mymsgbuf {
	long    mtype;          
	int id;
} mymsgbuf;

void conv_ref_pg_no(char * refs) //convert reference string to page number array
{
	const char s[2] = "|";
	char *token;
	token = strtok(refs, s);
	while ( token != NULL )
	{
		pg_no[no_of_pages] = atoi(token);
		no_of_pages++;
		token = strtok(NULL, s);
	}
}

int Read_msg_from_mmu( int qid, long type, struct mmumsgbuf_recv *qbuf ) // read message from MMU
{
	int result, length;
 
      // length will be size of the structure minus sizeof(mtype)
	length = sizeof(struct mmumsgbuf_recv) - sizeof(long);

	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}

int Send_msg_To_mmu( int qid, struct mmumsgbuf_send *qbuf ) // send message to MMU
{
	int result, length;

	 // length will be size of the structure minus sizeof(mtype)
	length = sizeof(struct mmumsgbuf_send) - sizeof(long);

	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (result);
}


int Send_msg_To_Scheduler( int qid, struct mymsgbuf *qbuf ) // send message to scheduler
{
	int     result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (result);
}
int Read_msg_from_Scheduler( int qid, long type, struct mymsgbuf *qbuf ) // read message from scheduler
{
	int     result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}

int main(int argc, char *argv[]) 
{
	//argv[] ={id,mq1,mq3,ref_string}
	if (argc < 5)
	{
		perror("Please give 5 arguments {id,mq1,mq3,ref_string}\n");
		exit(1);
	}
	int id, mq1_k, mq3_k;

	id = atoi(argv[1]);
	mq1_k = atoi(argv[2]);
	mq3_k = atoi(argv[3]);

	no_of_pages = 0;

	conv_ref_pg_no(argv[4]);

	int mq1, mq3;
	mq1 = msgget(mq1_k, 0666); //create message queue
	mq3 = msgget(mq3_k, 0666); // create message queue

	if (mq1 == -1)
	{
		perror("Message Queue1 creation failed");
		exit(1);
	}
	if (mq3 == -1)
	{
		perror("Message Queue3 creation failed");
		exit(1);
	}
	printf("Process id= %d\n", id);

	//sending to scheduler
	mymsgbuf msg_send;
	msg_send.mtype = TOSCH;
	msg_send.id = id;
	Send_msg_To_Scheduler(mq1, &msg_send);
	

	//Wait until msg receive from scheduler
	mymsgbuf msg_recv;
	Read_msg_from_Scheduler(mq1, FROMSCH + id, &msg_recv);
	

	mmumsgbuf_send mmu_send;
	mmumsgbuf_recv mmu_recv;

	int count_for_pageno = 0; //counter for page number array
	while (count_for_pageno < no_of_pages)
	{
		// sending msg to mmu the page number
		printf("Sent request for %d page number\n", pg_no[count_for_pageno]);
		mmu_send.mtype = TOMMU;
		mmu_send.id = id;
		mmu_send.pageno = pg_no[count_for_pageno];
		Send_msg_To_mmu(mq3, &mmu_send);

		Read_msg_from_mmu(mq3, FROMMMU + id, &mmu_recv);
		if (mmu_recv.frameno >= 0)
		{
			printf("Frame number from MMU received for process %d: %d\n" , id, mmu_recv.frameno);
			count_for_pageno++;
		}
		else if (mmu_recv.frameno == -1) //here count_for_pageno will not be incremented
		{
			printf("Page fault occured for process %d\n", id);
			
		}
		else if (mmu_recv.frameno == -2)
		{
			printf("Invalid page reference for process %d terminating ...\n", id) ;
			exit(1);
		}
	}
	printf("Process %d Terminated successfly\n", id);
	mmu_send.pageno = -9;
	mmu_send.id = id;
	mmu_send.mtype = TOMMU;
	Send_msg_To_mmu(mq3, &mmu_send);

	exit(1);
	return 0;
}


