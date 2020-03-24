#include"reactor.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


int gevent_set(struct gevent*ev,int fd,GCALLBACK callback,void*arg){

    ev->fd=fd;
    ev->callback=callback;
    ev->events=0;
    
    ev->arg=arg;
    ev->last_active=time(NULL);

    return 0;
}

//添加到reactor
int gevent_add(int epfd,int events,  struct gevent*ev){
   
    //ev->fd   ===>  add epoll

    struct epoll_event ep_ev={0,{0}};
    ep_ev.data.ptr=ev;
    ep_ev.events=ev->events=events;
    
    int oper;
    if(ev->status==1){ //已经在epoll里面
        oper=EPOLL_CTL_MOD;
    }
    else{
        oper=EPOLL_CTL_ADD;
        ev->status=1;
    }
    epoll_ctl(epfd,oper,ev->fd,&ep_ev);
   
    return 0;
    
}

int gevent_del(int epfd,struct gevent*ev){

    struct epoll_event ep_ev={0};

    if(ev->status!=1){return -1;}

    ep_ev.data.ptr=ev;
    ev->status=0;
    epoll_ctl(epfd,EPOLL_CTL_DEL,ev->fd,&ep_ev);

    return 0;

}

//epfd
//events
int greactor_init(struct greactor*reactor){
    if(reactor==NULL){return -1;}
    memset(reactor,0,sizeof(struct greactor));

    reactor->epfd=epoll_create(1);

    if(reactor->epfd<0){
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
        return -2;
    }

    reactor->events=(struct gevent*)malloc(MAX_EPOLL_EVENTS*sizeof(struct gevent));

    if(reactor->events==NULL){
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
        close(reactor->epfd);
        return -3;
    }

    return 0;
}


int greactor_run(struct greactor*reactor){
    if(reactor==NULL) return -1;
    if(reactor->epfd<0) return -1;
    if(reactor->events==NULL) return -1;


    struct epoll_event events[MAX_EPOLL_EVENTS+1]={0};  
    int checkpos = 0;

    while (1){
        
        //check timeout---> last active
        long now = time(NULL);
		for (size_t i = 0;i < 100;i ++, checkpos ++) {
			if (checkpos == MAX_EPOLL_EVENTS) {
				checkpos = 0;
			}

			if (reactor->events[checkpos].status != 1) {
				continue;
			}

			long duration = now - reactor->events[checkpos].last_active;

			if (duration >= 60) {
				close(reactor->events[checkpos].fd);
				printf("[fd=%d] timeout\n", reactor->events[checkpos].fd);
				gevent_del(reactor->epfd, &reactor->events[checkpos]);
			}
		}
       
        //epoll_wait
        int nready=epoll_wait(reactor->epfd,events,MAX_EPOLL_EVENTS,1000);
        if(nready<0) {
            printf("epoll_wait error, exit\n");
            continue;
        }
        for (size_t i = 0; i < nready; i++){
      
            struct gevent*ev=(struct gevent*)events[i].data.ptr;
            //epoll的socketIO可读 且 reactor关注的socketIO也是可读 （不关注的不读）
            if((events[i].events&EPOLLIN)&&(ev->events&EPOLLIN)){
                ev->callback(ev->fd,events[i].events,ev->arg);
            }

            if((events[i].events&EPOLLOUT&&(ev->events&EPOLLOUT))){
                ev->callback(ev->fd,events[i].events,ev->arg);
            }

        }
    }
    

    return 0;

}


int greactor_destory(struct greactor*reactor){
    if(reactor==NULL){return -1;}
    close(reactor->epfd);
    free(reactor->events);
    return 0;
}


int greactor_addlistener(struct greactor*reactor,int listenfd,GCALLBACK acceptor){
    
    if(reactor==NULL) return -1;
    if(reactor->events==NULL) return -1;

    gevent_set(&reactor->events[listenfd],listenfd,acceptor,reactor);
    gevent_add(reactor->epfd,EPOLLIN,&reactor->events[listenfd]);
    
    return 0;

}



int send_callback (int clientfd,int events,void*arg){
    struct greactor*reactor=(struct greactor*)arg;   if(reactor==NULL)return -1;
    struct gevent*ev=reactor->events+clientfd;

    int length=send(clientfd,ev->sbuffer,ev->slength,0);

    if(length>0){

        printf("send[fd=%d], [%d]%s\n", clientfd, length, ev->rbuffer);

        gevent_del(reactor->epfd,ev);
        gevent_set(ev,clientfd,recv_callback,reactor);
        gevent_add(reactor->epfd,EPOLLIN,ev);
    }
    else{
        close(ev->fd);
        gevent_del(reactor->epfd,ev);
        printf("send[fd=%d] error %s\n", clientfd, strerror(errno));
    }

    return length;
    
}

int recv_callback (int clientfd,int events,void*arg){

    struct greactor*reactor=(struct greactor*)arg;   if(reactor==NULL)return -1;
    struct gevent*ev=reactor->events+clientfd;
    
    memset(ev->rbuffer,0,BUFFER_LENGTH);
    int length=recv(clientfd,ev->rbuffer,BUFFER_LENGTH,0);
    ev->rlength=length;
    gevent_del(reactor->epfd,ev);

    if(length<0){    //recv时候数据没有准备好
        close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", clientfd, errno, strerror(errno));
    }
    else if(length==0){ //断开连接
        close(ev->fd);
        printf("[fd=%d] pos[%ld], closed\n", clientfd, ev-reactor->events);
    }
    else{ //收到数据
        ev->rlength=length;
        ev->rbuffer[length]='\0';

        printf("client[%d]: %s\n",clientfd,ev->rbuffer);

        //http_handler()--->  decode--->rbuffer ,  encode-->sbuffer
        reactor->handler(ev);

        gevent_set(ev,clientfd,send_callback,reactor);
        gevent_add(reactor->epfd,EPOLLOUT,ev);
    }

    return length;
    

}

int accpet_callback (int listenfd,int events,void*arg){
    struct greactor*reactor=(struct greactor*)arg; if(reactor==NULL)return -1;

    struct sockaddr_in  client_addr;
    memset(&client_addr,0,sizeof(struct sockaddr_in));
    socklen_t client_len=sizeof(client_addr);

    int clientfd= accept(listenfd,(struct sockaddr*)(&client_addr),&client_len);
    if(clientfd==-1){ //三次握手失败
        if(errno!=EAGAIN&&errno!=EINTR){
           
        }
       	printf("accept: %s\n", strerror(errno));
        return -1;
    }

    int i = 0;
	do {
		
		for (i = 0;i < MAX_EPOLL_EVENTS;i ++) {
			if (reactor->events[i].status == 0) {
				break;
			}
		}
		if (i == MAX_EPOLL_EVENTS) {
			printf("%s: max connect limit[%d]\n", __func__, MAX_EPOLL_EVENTS);
			break;
		}

		int flag = 0;
		if ((flag = fcntl(clientfd, F_SETFL, O_NONBLOCK)) < 0) {
			printf("%s: fcntl nonblocking failed, %d\n", __func__, MAX_EPOLL_EVENTS);
			break;
		}

		gevent_set(&reactor->events[clientfd], clientfd, recv_callback, reactor);
		gevent_add(reactor->epfd, EPOLLIN, &reactor->events[clientfd]);

	} while (0);

    printf("new connect [%s:%d][time:%ld], pos[%d]\n", 
		inet_ntoa(client_addr.sin_addr), 
        ntohs(client_addr.sin_port), 
        reactor->events[i].last_active, i);

    return 0;

}

/**************************************************************************/

int init_sock(short listenPort){
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    fcntl(listenfd,F_SETFL, O_NONBLOCK);  //设置为非阻塞

    struct sockaddr_in server_addr={0};
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(listenPort);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(listenfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in))==-1){
        perror("bind");
        return -1;
    }

    if(listen(listenfd,5)<0){ //backlog
       printf("listen failed : %s\n", strerror(errno));
       return -4;
    }

    return listenfd;

}

int greactor_setup(GHANDLER handler){

    unsigned short port = SERVER_PORT;

    int listenfd=init_sock(port);

    struct greactor *reactor = (struct greactor*)malloc(sizeof(struct greactor));
    greactor_init(reactor);
    reactor->handler=handler;

    greactor_addlistener(reactor,listenfd,accpet_callback);
    greactor_run(reactor);

    greactor_destory(reactor);

    close(listenfd);
 
}