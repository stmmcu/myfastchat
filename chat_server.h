#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#define BUFSIZE 65535

#define PORT    9002

struct 
{
	int len;
	char buf[65535];
}TcpPacket;

typedef struct worker
{

	void *(*process)(void *arg, char* buf);
	void *arg;
	char buf[255];
	struct worker *next;
}CThread_worker;
typedef struct param
{
	int *epoll_fd;
	int *accept_fd;
	int *maxfd;
        struct epoll_event *ev;
		
}CThread_param;
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
    
    CThread_worker *queue_head;
   
    int shutdown;
    pthread_t *threadid;
    
    int max_thread_num;
    
    int cur_queue_size;
} CThread_pool;
void *myprocess(void *arg, char* buf);
void pool_init(int);
int pool_add_worker (void *(*process) (void *arg, char*buf), void *arg);
void *thread_routine (void *arg);
 
static CThread_pool *pool = NULL;

int buildConnect(); 
long int  fd_A[5]= {0,0,0,0}; //because fd_A maybe flow 
int acceptSelectClient(int );

int acceptEpollClient(int);
int set_noblocking(int);
int readn(int, char* ,size_t);
int readrec(int ,char*,size_t);
//int pthread_test();
//void* thread_fun(void*);
//int reicveData();



//int toQueue();

//int dealData();

//int sendData();



