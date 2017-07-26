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

int main(int argc, char const **argv)
{
  struct sockaddr_in     serv_addr;
  char                   buf[MAXLINE];
  int                    sock_id;
  int                    recv_len;
  int                    write_len;
  int                    send_len;
  int                    read_len;
  int                    clie_send_count=0;
  int                    clie_recv_count=0;
  socklen_t              serv_addr_len;
  FILE                   *fp;
  if (argc<3) 
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
  inet_pton(AF_INET,argv[1],&serv_addr.sin_addr);
  
  serv_addr_len=sizeof(serv_addr);

  for(int i=2;i<argc;i++)
  {
     if((fp=fopen(argv[i],"r"))==NULL)
     {
         perror("Open file failed\n");
         exit(0);
     }

     while((read_len=fread(buf,sizeof(char),MAXLINE,fp))>0) 
     {
         send_len=sendto(sock_id,buf,read_len,0,(struct sockaddr*)&serv_addr,serv_addr_len);  
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
         clie_send_count++;  
         bzero(buf, MAXLINE);  
     }
  }
  sprintf(buf,"stop");
  sendto(sock_id,buf,MAXLINE,0,(struct sockaddr*)&serv_addr,serv_addr_len);
  clie_send_count++; 
  printf("Client have send %d packages!\n",clie_send_count);  
  
  /*if((fp=fopen(argv[argc-1],"a+"))==NULL)
  {
      perror("Open file failed\n");
      exit(0);
  }
  while(true)
  {
      bzero(buf,MAXLINE);
      recv_len=recvfrom(sock_id,buf,MAXLINE,0,(struct sockaddr *)&serv_addr,&serv_addr_len);
      if(recv_len<0)
      {
          perror("Receive file failed\n");
          exit(0);
          //break;
      }
      else if(recv_len==0)
      {
          printf("Server close sending file\n");
          break;
      }
      else
      {
          write_len = fwrite(buf, sizeof(char), recv_len,fp);
          if(write_len<recv_len)
          {
            perror("Write file failed\n");
            exit(0);
            //break;
          }
          clie_recv_count++;
      }
  }
  printf("Client have receive %d packages\n",clie_recv_count);
  printf("Received success!\n");*/
  fclose(fp);
  close(sock_id);
  printf("Cilent close\n");
  return 0;
}