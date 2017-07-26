#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>   
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  

#define    MAXLINE        8
#define    true           1
#define    port           1234 
   
int main(int argc,char **argv)   
{  
  struct sockaddr_in     serv_addr;  
  struct sockaddr_in     clie_addr;  
  char                   buf[MAXLINE];  
  int                    sock_id;  
  int                    link_id;  
  int                    recv_len;  
  int                    write_len;
  int                    send_len;
  int                    serv_recv_count=0;
  int                    serv_send_count=0;  
  socklen_t              clie_addr_len;  
  FILE                   *fp;
  
  if(argc!=2) 
  {  
      printf("Wrong input\n");  
      exit(0);
  }  

  if((sock_id = socket(AF_INET, SOCK_STREAM, 0))<0) 
  {  
      perror("Create socket failed\n");  
      exit(0);  
  }
  memset(&serv_addr, 0, sizeof(serv_addr));  
  serv_addr.sin_family = AF_INET;  
  serv_addr.sin_port = htons(port);  
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
  
  if(bind(sock_id,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0) 
  {  
      perror("Bind socket failed\n");  
      exit(0);  
  }
  if(listen(sock_id, 10)<0) 
  {  
      perror("Listen socket failed\n");  
      exit(0);  
  }
  while(true)
  {
     if((fp=fopen(argv[1],"a+"))==NULL) 
     {  
         perror("Open file failed\n");  
         exit(0);  
     }  
     clie_addr_len=sizeof(clie_addr); 

     link_id=accept(sock_id,(struct sockaddr *)&clie_addr,&clie_addr_len);  
     if(link_id<0)
     {  
         perror("Accept socket failed\n");  
         exit(0);  
     }  
     bzero(buf, MAXLINE);  
     while(recv_len=recv(link_id,buf,MAXLINE,0)) 
     {  
         if(recv_len < 0) 
         {  
            printf("Recieve data from client failed!\n");  
            break;  
         }  
         write_len=fwrite(buf,sizeof(char),recv_len,fp);  
         if (write_len<recv_len) 
         {  
            printf("Write file failed\n");  
            break;  
         }
         serv_recv_count++;
         send_len=send(link_id,buf,recv_len,0);
         if(send_len<0) 
         {  
           printf("Send file failed\n");  
           break;  
         }
         serv_send_count++;
         bzero(buf,MAXLINE);  
     }
     printf("Server have received %d packages and sent %d packages!\n",serv_recv_count,serv_send_count);
     fclose(fp);
     close(link_id); 
   } 
   printf("\nServer close!\n");       
   close(sock_id);   
   return 0;  
 }   