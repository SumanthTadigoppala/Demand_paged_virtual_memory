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
#define MAX_PROCESS 1000 //	Maximum processes
#define FROMPROCESS 10 // FROMPROCESS+id will be used to read msg from process
#define TOPROCESS 20  // TOPROCESS+id will be used to read msg from sch

#define FROMMMU 20 // FROMMMU+id will be used to read msg from MMU

#define PAGEFAULT_HANDLED 5 // Page fault handled
#define TERMINATED 10 // Process terminated

int k; //no. of processes

typedef struct _mmutosch {
	long    mtype;
	char mbuf[1];
} mmutosch;



typedef struct mymsgbuf {
	long    mtype;
	int id;
} mymsgbuf;





int Read_msg_From_Process( int qid, long type, struct mymsgbuf *qbuf ) // Read message from Process
{
	int     result, length;

     // length will be size of the structure minus sizeof(mtype)
	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}

int Send_msg_TO_Process( int qid, struct mymsgbuf *qbuf ) // Send message to Process
{
	int     result, length;

    // length will be size of the structure minus sizeof(mtype)
 	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (result);
}

int Read_msg_From_MMU( int qid, long type,mmutosch *qbuf ) // Read message from MMU
{
	int result, length;

	// length will be size of the structure minus sizeof(mtype)
	length = sizeof(mmutosch) - sizeof(long);

	if ((result = msgrcv(qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}


int main(int argc , char * argv[])
{
	int mq1_key, mq2_key, master_pid;
	if (argc < 5) {
		printf("Scheduler  argc < 5\n");
		exit(EXIT_FAILURE);
	}

	mq1_key = atoi(argv[1]);
	mq2_key = atoi(argv[2]);
	k = atoi(argv[3]);
	master_pid = atoi(argv[4]);


	mymsgbuf msg_send, msg_recv;

	int mq1 = msgget(mq1_key, 0666);
	int mq2 = msgget(mq2_key, 0666);
	if (mq1 == -1)
	{
		perror("Message Queue1 creation failed");
		exit(1);
	}

	if (mq2 == -1)
	{
		perror("Message Queue2 creation failed");
		exit(1);
	}
	
	printf("Total No. of Process received = %d\n", k);

	
	int terminated_process = 0; //to keep track of running process to exit at last
	
	while (1)
	{
		Read_msg_From_Process(mq1, FROMPROCESS, &msg_recv);
		int curr_id = msg_recv.id;

		msg_send.mtype = TOPROCESS + curr_id;
		Send_msg_TO_Process(mq1, &msg_send);

		//recv messages from mmu
		mmutosch mmu_recv;
		Read_msg_From_MMU(mq2, 0, &mmu_recv);
	
		if (mmu_recv.mtype == PAGEFAULT_HANDLED)
		{
			msg_send.mtype = FROMPROCESS;
			msg_send.id=curr_id;
			Send_msg_TO_Process(mq1, &msg_send);
		}
		else if (mmu_recv.mtype == TERMINATED)
		{
			terminated_process++;
			
		}
		else
		{
			perror("Wrong message from mmu\n");
			exit(1);
		}
		if (terminated_process == k)
			break;
		
	}

	kill(master_pid, SIGUSR1);
	pause();
	printf("Scheduler terminating ...\n") ;
	exit(1) ;
}