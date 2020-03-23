/**
 * 非阻塞IO: 一请求一线程方式
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>


#define BUFFER_LENGTH 128


void* clientCallback(void*arg){
    int clientfd=*(int*)arg;
    while (1) {
       char buffer[BUFFER_LENGTH];
       int length=recv(clientfd,buffer,BUFFER_LENGTH,0);

       if(length<0){
           usleep(1); 
           continue;
       }
       else if(length==0){
           break;
       }
       

       printf("%s\n",buffer);
    }
    return NULL;
    
}

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

 
    fcntl(listenfd,F_SETFL, O_NONBLOCK);

    while (1) {
        struct sockaddr_in  client_addr;
        memset(&client_addr,0,sizeof(struct sockaddr_in));
        socklen_t client_len=sizeof(client_addr);
        int clientfd= accept(listenfd,(struct sockaddr*)(&client_addr),&client_len);

        if(clientfd<=0){ usleep(1); continue;}
       
        fcntl(clientfd,F_SETFL,O_NONBLOCK);
           
        pthread_t thread_id;
        int ret= pthread_create(&thread_id,NULL,clientCallback,&clientfd);
        if(ret<0){
            exit(-5);
        }

        usleep(1);
        
    }
    

    return 0;
}