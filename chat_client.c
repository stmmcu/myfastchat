#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define BUFSIZE 8192
#define DEFAULT_PORT 9002 

struct 
{
	int len;
	char buf[65535];
}TcpPacket;
enum {CMD_NAME, DST_IP, DST_PORT};

int main(int argc, char *argv[])
{
        struct sockaddr_in server;
        unsigned long dst_ip;
        int port;
        int s;
        int n;
        char buf[BUFSIZE];
        char buf1[BUFSIZE];
        char cmd[BUFSIZE];
        struct timeval tv;
        fd_set readfd;

        port = DEFAULT_PORT;
        dst_ip = inet_addr("10.207.11.139");
        if((s = socket(AF_INET,SOCK_STREAM, 0)) < 0)
        {
                perror("sokcet error");
                exit(-1);
        }
        memset((char *)&server,0,sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr=dst_ip;
        server.sin_port = htons(port);

        if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
                perror("connect error");
                exit(-1);
        }
          while(1)
        {
                tv.tv_sec = 600;
                tv.tv_usec = 0;
                FD_ZERO(&readfd);
                FD_SET(0,&readfd);
                FD_SET(s, &readfd);
                if((select(s+1, &readfd, NULL, NULL, &tv)) < 0)
                {
                        perror("timeout");
                        break;
                }
                if(FD_ISSET(0, &readfd))
                {
                        if(( n = read(0, TcpPacket.buf,  sizeof(TcpPacket.buf))) <= 0)
                                break;
			n = strlen(TcpPacket.buf);
		//	TcpPacket.buf[n-1] = '\0';
			TcpPacket.len =ntohl(n);
                        if(send(s,(char *)&TcpPacket,  n+ sizeof(TcpPacket.len), 0) <= 0)
                                break;
                }
                if(FD_ISSET(s, &readfd))
                {
                        if(( n = recv(s, buf1, BUFSIZE-1,0)) <=0 )
                        {
                                perror("recv error!\r\n");
                                exit(-1);
                        }
                        buf1[n] = '\0';
                        printf("%s",buf1);
                        fflush(stdout);
                }
        }
        close(s);
        return 0;
}
