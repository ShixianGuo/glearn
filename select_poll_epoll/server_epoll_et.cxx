/**
 * IO复用： epoll
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
#include <sys/epoll.h> 

#define BUFFER_LENGTH 128
#define EPOLL_SIZE 1024

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

    int epfd=epoll_create(1);
    struct epoll_event ev;   //要处理的事件
    struct epoll_event events[EPOLL_SIZE]={0};   //返回的要处理的事件合集存放地
    
    ev.events=EPOLLIN;
    ev.data.fd=listenfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    
    //小区快递点模型
    while (1) {
        int nready=epoll_wait(epfd,events,EPOLL_SIZE,-1); //epoll -1 一直阻塞 到有数据
        if(nready==-1){
            continue;
        }

        for (size_t i = 0; i < nready; i++){
            //listen->fd   accept
            if(events[i].data.fd==listenfd){
               struct sockaddr_in  client_addr;
               memset(&client_addr,0,sizeof(struct sockaddr_in));
               socklen_t client_len=sizeof(client_addr);
               int clientfd= accept(listenfd,(struct sockaddr*)(&client_addr),&client_len);
               if(clientfd<=0){  continue;}

               ev.events=EPOLLIN|EPOLLOUT;
               ev.data.fd=clientfd;
               epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ev);

            }
            //client->fd   recv/send
            else 
            {
                char buffer[BUFFER_LENGTH]={0};
                int length=recv(i,buffer,BUFFER_LENGTH,0);

                if(length<0){
                    if(errno==EAGAIN||errno==EWOULDBLOCK){
                        printf("read all data");
                    }
                    close(i);
                    ev=events[i];
                    epoll_ctl(epfd,EPOLL_CTL_DEL,i,&ev);

                }
                else if(length==0){ //正常断开一个连接
                    printf("disconnect %d\n",i);
                    close(i);
                    ev=events[i];
                    epoll_ctl(epfd,EPOLL_CTL_DEL,i,&ev);
                    break;
                }
                else{
                    printf("%s\n",buffer);
                }
            }
            

            

        }
        
        
    }
    





    
    

    

      



    
    




  
    

    return 0;
}