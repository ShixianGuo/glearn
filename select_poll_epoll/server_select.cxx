/**
 * IO复用： select
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

#define BUFFER_LENGTH 128


int main(int argc,char**argv){
    
    if(argc<2){
        printf("paramter error\n");
        return -1;
    }

    int listenPort=atoi(argv[1]);
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0){
        perror("socket");
        return -2;
    }


    struct sockaddr_in addr;

    addr.sin_family=AF_INET;
    addr.sin_port=htons(listenPort);
    addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(listenfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))<0){
        perror("bind");
        return -3;
    }

    if(listen(listenfd,5)<0){ //backlog
       perror("listen");
       return -4;
    }

    //把监听的和接受的fd放入 rfds    
    //每次循环开始 重置rset（rset=rfds） 传入内核
    fd_set rfds,rset;  
    FD_ZERO(&rfds);
    FD_SET(listenfd,&rfds); //fd对应位置1

    int max_fd=listenfd;
    while (1){

      rset=rfds;

      int nready=select(max_fd+1,&rset,NULL,NULL,NULL); //时间为NULL 没有数据 一直阻塞

      if(nready<0) continue;  

      //listen->fd   accept
      if(FD_ISSET(listenfd,&rset)){

        struct sockaddr_in  client_addr;
        memset(&client_addr,0,sizeof(struct sockaddr_in));
        socklen_t client_len=sizeof(client_addr);
        int clientfd= accept(listenfd,(struct sockaddr*)(&client_addr),&client_len);

        if(clientfd<=0){  continue;}
        
        if(max_fd==FD_SETSIZE){
            printf("out range\n");
            break;
        }

        FD_SET(clientfd,&rfds);
        if(--nready==0)continue;
      }
      //client->fd   recv/send
      for (size_t i = listenfd+1; i < max_fd; i++){
         if(FD_ISSET(i,&rset)){

            char buffer[BUFFER_LENGTH]={0};
            int length=recv(i,buffer,BUFFER_LENGTH,0);

            if(length<0){
                if(length<0){
                if(errno==EAGAIN||errno==EWOULDBLOCK){
                    printf("read all data");
                }
                close(i);
                FD_CLR(i,&rfds);
            }
            else if(length==0){ //断开一个连接
                printf("disconnect %d\n",i);
                close(i);
                FD_CLR(i,&rfds);
                break;
            }
            else{
                printf("%s\n",buffer);
            }

            if(--nready==0)break;
        
         
         }
      }
      



    }
    




  
    

    return 0;
}