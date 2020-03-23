/**
 * IO复用： poll
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
#include <poll.h> 

#define BUFFER_LENGTH 128
#define POLL_SIZE 128

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

    struct pollfd pfds[POLL_SIZE];
    pfds[0].fd=listenfd;
    pfds[0].events=POLLIN;
    
    int max_fd=0; 
    for (size_t i = 01; i < POLL_SIZE; i++){
        pfds[i].fd=-1;
    }
    
    while (1)
    {
        int nready=poll(pfds,max_fd+1,5);
        if(nready<=0) continue;

        //listen->fd   accept
        if((pfds[0].revents&POLLIN)==POLLIN){
           struct sockaddr_in  client_addr;
           memset(&client_addr,0,sizeof(struct sockaddr_in));
           socklen_t client_len=sizeof(client_addr);
           int clientfd= accept(listenfd,(struct sockaddr*)(&client_addr),&client_len);
           if(clientfd<=0){  continue;}

           pfds[clientfd].fd=clientfd;
           pfds[clientfd].events=POLLIN;

           if(clientfd>max_fd) max_fd=clientfd;
           if(--nready==0)continue;

        }
        //client->fd   recv/send
        for (size_t i = listenfd+1; i < max_fd; i++){
            if(pfds[i].revents&(POLLIN|POLLERR)){
                char buffer[BUFFER_LENGTH]={0};
                int length=recv(i,buffer,BUFFER_LENGTH,0);

                if(length<0){
                    if(errno==EAGAIN||errno==EWOULDBLOCK){
                        printf("read all data");
                    }
                    close(i);
                    pfds[i].fd=-1;
                }
                else if(length==0){ //断开一个连接
                    printf("disconnect %d\n",i);
                    close(i);
                    pfds[i].fd=-1;
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