#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAXLINE 8
#define true    1
#define port    1234

struct package
{
   int  num;
   char buf[MAXLINE];
};

int main(int argc, char const **argv)
{
	struct sockaddr_in     serv_addr;
	struct sockaddr_in     clie_addr;
	struct package         pack;
	int                    sock_id;
    int                    recv_len;
    int                    write_len;
	int                    serv_count=0;
    socklen_t              clie_addr_len;
    FILE                   *fp1;
    //FILE                 *fp2;
	if(argc!=2) 
    {  
        printf("Wrong input\n");  
        exit(0);
    }

    sock_id=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_id<0)
    {
    	perror("Create socket failed\n");
    	exit(0);
    }
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr= htonl(INADDR_ANY);
    if(bind(sock_id,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
    {
    	perror("Bind socket failed\n");
    	exit(0);
    }

    if ((fp1 = fopen(argv[1],"a+")) == NULL) 
    {  
        perror("Open file failed\n");  
        exit(0);  
    }
    clie_addr_len=sizeof(clie_addr);
    bzero(pack.buf,MAXLINE);     
    while(true)
    {
       recv_len=recvfrom(sock_id,(char *)&pack,sizeof(struct package),0,(struct sockaddr *)&clie_addr,&clie_addr_len);
       if(recv_len<0)
       {
         perror("Receive file failed!\n");
         exit(0);
       }
       else if(strncmp(pack.buf,"stop",4)==0)
       {
         serv_count++;
         break;
       }
       //fwrite((char *)&(pack[serv_count].num),sizeof(int),1,fp1);
       else
       {
         write_len = fwrite((char *)&(pack.buf),sizeof(char),MAXLINE,fp1);
         bzero(pack.buf,MAXLINE);
         serv_count++;
       }
    }
    printf("Server have received %d packages!\n",serv_count);
    //bzero(buf,MAXLINE);
    //serv_recv_count++;
    //printf("Server have received %d packets!\n",serv_recv_count);
    fclose(fp1);
    
    /*if ((fp2 = fopen(argv[1],"r")) == NULL) 
    {  
        perror("Open file failed\n");  
        exit(0);  
    }  
    while ((read_len = fread(buf, sizeof(char), MAXLINE, fp2)) >0) 
    {
        send_len = sendto(sock_id, buf,read_len, 0,(struct sockaddr*)&clie_addr,clie_addr_len);  
        if (send_len < 0 ) 
        {  
            perror("Send file failed\n");  
            exit(0);  
        }
        if(send_len<read_len)
        {
            perror("Send not enough file\n");
            exit(0);
        }
        serv_send_count++;  
        bzero(buf, MAXLINE);  
    } 
    printf("Server have send %d packages!\n",serv_send_count);
    fclose(fp2); */ 
    close(sock_id);
    printf("Server close\n"); 
    return 0;
}