#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
   
#define   MAXLINE     8
#define   true        1
#define   port        1234  
   
int main(int argc,char **argv)  
{  
    struct sockaddr_in     serv_addr;  
    char                   buf[MAXLINE];  
    int                    sock_id;  
    int                    read_len;  
    int                    send_len;
    int                    recv_len;
    int                    write_len;    
    int                    connect_id; 
    int                    client_send_count=0;
    int                    client_recv_count=0;
    FILE                   *fp1;
    FILE                   *fp2;
    if(argc<3) 
    {  
        printf("Wrong input\n");  
        exit(0); 
    }  
       
    if((sock_id=socket(AF_INET,SOCK_STREAM,0))<0) 
    {  
        perror("Create socket failed\n");  
        exit(0);  
    }  
    memset(&serv_addr,0,sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_port = htons(port);  
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);  
      
    connect_id=connect(sock_id,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr));  
    if(connect_id<0) 
    {  
        perror("Connect socket failed\n");  
        exit(0);  
    }
    for(int i=2;i<argc-1;i++)
    {
       if((fp1=fopen(argv[i],"r"))==NULL) 
       {  
           perror("Open file failed\n");  
           exit(0);  
       }
       if((fp2=fopen(argv[argc-1],"a+"))==NULL) 
       {  
           perror("Open file failed\n");  
           exit(0);  
       }     
       bzero(buf, MAXLINE);  
       while((read_len=fread(buf,sizeof(char),MAXLINE,fp1))>0) 
       {
           send_len=send(sock_id,buf,read_len,0);  
           if(send_len < 0 ) 
           {  
             printf("Send file failed\n");  
             break;  
           }
           if(send_len<read_len)
           {
             printf("Send not enough file\n");
             break;
           }
           client_send_count++;
           bzero(buf,MAXLINE);
           recv_len = recv(sock_id,buf,MAXLINE,0);
           if(recv_len < 0) 
           {  
               printf("Recieve Data From Server Failed!\n");  
               break;  
           }  
           write_len=fwrite(buf,sizeof(char),recv_len,fp2);  
           if(write_len<recv_len) 
           {  
               printf("Write file failed\n");  
               break;  
           }
           client_recv_count++;  
           bzero(buf, MAXLINE);  
       } 
    }  
    printf("Client have sent %d packages and received %d packages!\n",client_send_count,client_recv_count);
    fclose(fp1);
    fclose(fp2);  
    close(sock_id);  
    printf("Client close\n");  
    return 0;  
 }  