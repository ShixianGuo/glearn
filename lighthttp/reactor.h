
#ifndef __REACTOR_H__
#define __REACTOR_H__

#define BUFFER_LENGTH 4096
#define MAX_EPOLL_EVENTS 1024
#define SERVER_PORT 6666

typedef  int GCALLBACK (int ,int ,void*);
typedef  int GHANDLER(void*arg);

struct gevent{
    int fd;  
    int events;
    void* arg;
   
    int(*callback)(int fd,int events,void*arg);
    //GCALLBACK callback;

    char rbuffer[BUFFER_LENGTH];
    int  rlength;
    char sbuffer[BUFFER_LENGTH];
    int  slength;
    
    int status; //标识有没有在reactor内部
    long last_active; //fd最近一次活跃时间

};

struct greactor{
    int epfd;
    struct gevent*events;
    GHANDLER* handler;
};



int gevent_set(struct gevent*ev,int fd,GCALLBACK cb,void*arg);
int gevent_add(int epfd,int events,  struct gevent*ev);
int gevent_del(int epfd,struct gevent*ev);

int greactor_init(struct greactor*reactor);
int greactor_addlistener(struct greactor*reactor,int listenfd,GCALLBACK acceptor);
int greactor_run(struct greactor*reactor);
int greactor_destory(struct greactor*reactor);

int send_callback (int clientfd,int events,void*arg);
int recv_callback (int clientfd,int events,void*arg);
int accpet_callback (int listenfd,int events,void*arg);

int greactor_setup(GHANDLER handler);

#endif
