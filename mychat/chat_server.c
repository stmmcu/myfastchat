#include "chat_server.h"
int main()
{
	int fd;
	pool_init(3);
	fd = buildConnect();
	//acceptSelectClient(fd);	
	acceptEpollClient(fd);	
	return 0;
}

int buildConnect()
{
	struct sockaddr_in server;
	int listen_fd;
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;	
	}
	
    	
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(listen_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		return -1;
	}
	listen(listen_fd, 5);
	printf("server start listen....\r\n");
	return listen_fd;

}
int set_nonblocking(int listen_fd)
{
	if(fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFD, 0) | O_NONBLOCK) == -1)
	{
		return -1;
	}
	return 0;
}
/*
void * thread_fun(void *arg)
{
	pid_t pid;
	pthread_t tid;
	pid = getpid();
	tid = pthread_self();
	printf(" pid %u tid %u (0x%x)\n" ,(unsigned int)pid, (unsigned int )tid , (unsigned int ) tid);	
	return ((void *)1);
}
int pthread_test()
{
	int err;
	pthread_t tid;
	err = pthread_create(&tid, NULL , thread_fun, NULL);
	//err = pthread_join(tid , NULL);
	return 0;

}
*/
int readn(int accept_fd, char* buf, size_t len)
{
	int cnt;
	int rc;
	cnt  = len;
	while (cnt > 0)
	{
		rc = recv(accept_fd, buf, cnt, 0);	
		if(rc < 0)
		{
			return -1;
		}
		if(rc == 0)
			return len-cnt;
		buf += rc;
		cnt -= rc;
	}
	return len;
}
int readrec(int accept_fd, char* buf, size_t len)
{                     
        int reclen;   
        int rc;
        rc = readn(accept_fd, (char *)&reclen, sizeof(reclen));     
        if( rc != sizeof(reclen))
                return rc < 0 ? -1:0;            
        reclen = ntohl(reclen);
              
        if(reclen > len)                         
        {             
                while(reclen > 0)                
                {     
                        rc  = readn(accept_fd, buf , len);    
                        if(rc != len)            
                        return rc < 0 ? -1: 0;   
                        reclen -= len;           
                        if(reclen < len)         
                         len = reclen;
                }                                
        }
        rc = readn(accept_fd, buf, reclen);      
        if(rc != reclen)                         
                return rc < 0 ? -1 : 0;          
        return rc;                               
}

int acceptEpollClient(int listen_fd)
{
	int epoll_fd;	
	int maxfd;
	int ret;
	int accept_fd;
	int i;
	struct sockaddr_in client;
	struct epoll_event ev;
	struct epoll_event events[5];
	int len = sizeof(client);	
	char buf[255];
	if(set_nonblocking(listen_fd) < 0)
	{
		printf("fcntl error\r\n");
		return -1;
	} 
	epoll_fd = epoll_create(5);
	if(epoll_fd == 0)
	{
		printf("create error\r\n");	
		return -1;
	}
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listen_fd;
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
	{
		printf("epoll add error\r\n");
		return -1;
	}
	else
	{
		printf("listen id %d\r\n",(int)listen_fd);
	}	
	//set_keep_alive(listen_fd);
	maxfd = 1;
	while(1)
	{
		ret = epoll_wait(epoll_fd, events, maxfd, -1);

		if(ret == -1)
		{
			printf("wait\r\n");
			return -1;	
		}
		for(i = 0; i < ret ; i++)
		{
			if(events[i].data.fd == listen_fd)
			{
				accept_fd = accept(listen_fd, (struct sockaddr *)&client, &len) ;
				if(accept_fd < 0)
				{
					printf("accetp error\r\n");
					continue;
				}
				set_nonblocking(accept_fd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = accept_fd;
				if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &ev) < 0)
				{
					printf("epoll add error\r\n");
					return -1;
				}
				else
				{
					printf("new client %d\r\n", (int)accept_fd);

				maxfd++;
				}	
			}	
			else if(events[i].events&EPOLLIN)
			{
				CThread_param *CPThread_param = (CThread_param *) malloc (sizeof (CThread_param));
				CPThread_param->epoll_fd = &epoll_fd;
				CPThread_param->accept_fd = &(events[i].data.fd);	
				CPThread_param->ev = &ev;
				CPThread_param->maxfd = &maxfd;
				
				pool_add_worker (myprocess, CPThread_param);
				/*
				while(1)
				{
					ret = readrec(events[i].data.fd, buf,sizeof(buf));
				//	ret = recv(events[i].data.fd, buf, sizeof(buf),0);
					if(ret == -1)
					{
						if(errno == EAGAIN)
						{
							break;
							
						}
						
						break;
					}
					else if(ret == 0)
					{
						printf("client[%d] close\n",i);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,&ev);	
						maxfd--;
						break;
						
					}		
					else
					{

						buf[ret] = '\0';
						printf("client send %s\r\n", buf);
						
   
						send(events[i].data.fd,buf, sizeof(buf) , 0); 
						pthread_test();
					}
				}*/
			}

		}
	}
	close(listen_fd);
	return 0;
	
}
	
// select block way
int acceptSelectClient(int listen_fd)
{
	struct sockaddr_in client;
	int accept_fd;
	int maxfd;
	struct timeval tv;
	fd_set select_fd;
	int ret;
	int i;
	int len = sizeof(client);	
	int amount = 0;
	char buf[256];
	
	maxfd = listen_fd; 
	
	while(1)
	{
		FD_ZERO(&select_fd);
		FD_SET(listen_fd, &select_fd);
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		for(i = 0; i < 5; i++)
		{
			if(fd_A[i] != 0) { FD_SET(fd_A[i], &select_fd);}
		}
		ret = select(maxfd +1 , &select_fd, NULL, NULL, &tv);
		if(ret < 0)
		{
			return -1;
		}
		else if(ret == 0)
		{
			printf("timeout\r\n");
			continue;
		}
		for(i =0; i< amount; i++)
		{
			if(FD_ISSET(fd_A[i], &select_fd))
			{
				ret = recv(fd_A[i], buf, sizeof(buf), 0);
				if(ret <= 0)
				{
					printf("client[%d] close\n",i);
					close(fd_A[i]);
					FD_CLR(fd_A[i], &select_fd);
					fd_A[i] = 0;
					
				}		
				else
				{
					buf[ret] = '\0';
					printf("client send %s\r\n", buf);
				}
			}		
		}
		if(FD_ISSET(listen_fd, &select_fd))
		{
			printf("new connect come on \r\n");
			accept_fd = accept(listen_fd, (struct sockaddr *)&client, &len) ;
			if(accept_fd <= 0)
			{
				printf("accept error\r\n");
				continue;
			}
		
			if(amount < 5)
			{
				fd_A[amount++] = accept_fd;
				if(accept_fd > 	maxfd)
					maxfd = accept_fd;
			}	
			else
			{
				printf("max connects arrive\r\n");
			}
		}

	}
	for(i = 0; i < 5 ; i++)
	{
		if(fd_A[i] != 0) 
		{	
			close(fd_A[i]);
		}
	}	
	return 0;
}

void
pool_init (int max_thread_num)
{
    pool = (CThread_pool *) malloc (sizeof (CThread_pool));
    pthread_mutex_init (&(pool->queue_lock), NULL);
    pthread_cond_init (&(pool->queue_ready), NULL);
    pool->queue_head = NULL;
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;
    pool->threadid =
        (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));
    int i = 0;
    for (i = 0; i < max_thread_num; i++)
    { 
        pthread_create (&(pool->threadid[i]), NULL, thread_routine,
                NULL);
    }
}
int
pool_add_worker (void *(*process) (void *arg, char* buf), void *arg)
{
    
    CThread_worker *newworker =
        (CThread_worker *) malloc (sizeof (CThread_worker));
    newworker->process = process;
    newworker->arg = arg;
    newworker->next = NULL;
    pthread_mutex_lock (&(pool->queue_lock));
    
    CThread_worker *member = pool->queue_head;
    if (member != NULL)
    {
        while (member->next != NULL)
            member = member->next;
        member->next = newworker;
    }
    else
    {
        pool->queue_head = newworker;
    }
    assert (pool->queue_head != NULL);
    pool->cur_queue_size++;
	printf("cur_____%d\r\n", pool->cur_queue_size);
    pthread_mutex_unlock (&(pool->queue_lock));
    
    pthread_cond_signal (&(pool->queue_ready));
    return 0;
}
int
pool_destroy ()
{
    if (pool->shutdown)
        return -1;
    pool->shutdown = 1;
   
    pthread_cond_broadcast (&(pool->queue_ready));
   
    int i;
    for (i = 0; i < pool->max_thread_num; i++)
        pthread_join (pool->threadid[i], NULL);
    free (pool->threadid);
    
    CThread_worker *head = NULL;
    while (pool->queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free (head);
    }
    
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free (pool);
   
    pool=NULL;
    return 0;
}
 
void *
thread_routine (void *arg)
{
    printf ("starting thread 0x%x\n",(unsigned int) pthread_self ());
    while (1)
    {
        pthread_mutex_lock (&(pool->queue_lock));
       
        
        while (pool->cur_queue_size == 0 && !pool->shutdown)
        {
            printf ("thread 0x%x is waiting\n", (unsigned int) pthread_self ());
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));
        }
       
        if (pool->shutdown)
        {
           
            pthread_mutex_unlock (&(pool->queue_lock));
            printf ("thread 0x%x will exit\n", (unsigned int) pthread_self ());
            pthread_exit (NULL);
        }
        printf ("thread 0x%x is starting to work\n", (unsigned int) pthread_self ());
       
        assert (pool->cur_queue_size != 0);
        assert (pool->queue_head != NULL);
        
      
        pool->cur_queue_size--;
        CThread_worker *worker = pool->queue_head;
        pool->queue_head = worker->next;
        pthread_mutex_unlock (&(pool->queue_lock));
      	printf("start process...\r\n"); 
        (*(worker->process)) (worker->arg, worker->buf);
        free (worker);
        worker = NULL;
    }
   
    pthread_exit (NULL);
}

void *
myprocess (void *arg, char *buf)
{
	int ret;
	char buf1[255];
	struct param *PP_param ;
	PP_param = (CThread_param *) arg;
	printf("myprocess start...\r\n");
				while(1)
				{
					//printf("fd_%d\r\n",*(PP_param->accept_fd));
					ret = readrec(*(PP_param->accept_fd), buf1,sizeof(buf1));
				//	ret = recv(events[i].data.fd, buf, sizeof(buf),0);
					if(ret == -1)
					{
						if(errno == EAGAIN)
						{
							break;
							
						}
						
						break;
					}
					else if(ret == 0)
					{
						epoll_ctl(*(PP_param->epoll_fd), EPOLL_CTL_DEL, *(PP_param->accept_fd),PP_param->ev);	
						*(PP_param->maxfd)--;
						break;
						
					}		
					else
					{

						buf1[ret] = '\0';
						printf("client send %s\r\n", buf1);
						
						    printf ("threadid is 0x%x\n", (unsigned int) pthread_self ());
   
						send(*(PP_param->accept_fd),buf1, sizeof(buf1) , 0); 
					}
			
				}
				
    return NULL;
}
