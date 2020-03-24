


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_LENGTH		512
#define MAX_EPOLL_EVENTS	1024*1024
#define MAX_EPOLL_ITEM      1024*100
#define SERVER_PORT			8888

#define LISTEN_PORT_COUNT   100

typedef int GCALLBACK(int ,int, void*);

struct gevent {
	int fd;
	int events;
	void *arg;
	int (*callback)(int fd, int events, void *arg);
	
	int status;
	char buffer[BUFFER_LENGTH];
	int length;
	long last_active;
};



struct greactor {
	int epfd;
	struct gevent *events;
};

void gevent_set(struct gevent *ev, int fd, GCALLBACK callback, void *arg);
int  gevent_add(int epfd,int events,  struct gevent*ev);
int  gevent_del(int epfd,struct gevent*ev);

int greactor_init(struct greactor*reactor);
int greactor_addlistener(struct greactor*reactor,int listenfd,GCALLBACK acceptor);
int greactor_run(struct greactor*reactor);
int greactor_destory(struct greactor*reactor);

int send_callback (int clientfd,int events,void*arg);
int recv_callback (int clientfd,int events,void*arg);
int accept_callback (int listenfd,int events,void*arg);



void gevent_set(struct gevent *ev, int fd, GCALLBACK callback, void *arg) {

	ev->fd = fd;
	ev->callback = callback;
	ev->events = 0;
	ev->arg = arg;
	ev->last_active = time(NULL);

	return ;
	
}


int gevent_add(int epfd, int events, struct gevent *ev) {

	struct epoll_event ep_ev = {0, {0}};
	ep_ev.data.ptr = ev;
	ep_ev.events = ev->events = events;

	int op;
	if (ev->status == 1) {
		op = EPOLL_CTL_MOD;
	} else {
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if (epoll_ctl(epfd, op, ev->fd, &ep_ev) < 0) {
		printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
		return -1;
	}

	return 0;
}

int gevent_del(int epfd, struct gevent *ev) {

	struct epoll_event ep_ev = {0, {0}};

	if (ev->status != 1) {
		return -1;
	}

	ep_ev.data.ptr = ev;
	ev->status = 0;
	epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &ep_ev);

	return 0;
}

int recv_callback(int fd, int events, void *arg) {

	struct greactor *reactor = (struct greactor*)arg;
	struct gevent *ev = reactor->events+fd;

	int len = recv(fd, ev->buffer, BUFFER_LENGTH, 0);
	gevent_del(reactor->epfd, ev);

	if (len > 0) {
		
		ev->length = len;
		ev->buffer[len] = '\0';

		printf("C[%d]:%s\n", fd, ev->buffer);

		gevent_set(ev, fd, send_callback, reactor);
		gevent_add(reactor->epfd, EPOLLOUT, ev);
		
		
	} else if (len == 0) {

		close(ev->fd);
		printf("[fd=%d] pos[%ld], closed\n", fd, ev-reactor->events);
		 
	} else {

		close(ev->fd);
		printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
		
	}

	return len;
}


int send_callback(int fd, int events, void *arg) {

	struct greactor *reactor = (struct greactor*)arg;
	struct gevent *ev = reactor->events+fd;

	int len = send(fd, ev->buffer, ev->length, 0);
	if (len > 0) {
		printf("send[fd=%d], [%d]%s\n", fd, len, ev->buffer);

		gevent_del(reactor->epfd, ev);
		gevent_set(ev, fd, recv_callback, reactor);
		gevent_add(reactor->epfd, EPOLLIN, ev);
		
	} else {

		close(ev->fd);

		gevent_del(reactor->epfd, ev);
		printf("send[fd=%d] error %s\n", fd, strerror(errno));

	}

	return len;
}

int accept_callback(int fd, int events, void *arg) {

	struct greactor *reactor = (struct greactor*)arg;
	if (reactor == NULL) return -1;

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int clientfd;

	if ((clientfd = accept(fd, (struct sockaddr*)&client_addr, &len)) == -1) {
		if (errno != EAGAIN && errno != EINTR) {
			
		}
		printf("accept: %s\n", strerror(errno));
		return -1;
	}

	int i = 0;
	do {
#if 0		
		for (i = 0;i < MAX_EPOLL_EVENTS;i ++) {
			if (reactor->events[i].status == 0) {
				break;
			}
		}
		if (i == MAX_EPOLL_EVENTS) {
			printf("%s: max connect limit[%d]\n", __func__, MAX_EPOLL_EVENTS);
			break;
		}
#endif
		int flag = 0;
		if ((flag = fcntl(clientfd, F_SETFL, O_NONBLOCK)) < 0) {
			printf("%s: fcntl nonblocking failed, %d\n", __func__, MAX_EPOLL_EVENTS);
			break;
		}

		gevent_set(&reactor->events[clientfd], clientfd, recv_callback, reactor);
		gevent_add(reactor->epfd, EPOLLIN, &reactor->events[clientfd]);

	} while (0);

	printf("new connect [%s:%d][time:%ld], pos[%d]\n", 
		inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), reactor->events[i].last_active, i);

	return 0;

}

int init_sock(short port) {

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(fd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (listen(fd, 20) < 0) {
		printf("listen failed : %s\n", strerror(errno));
	}

	printf("listener port: [%d]\n",port);

	return fd;
}


int greactor_init(struct greactor *reactor) {

	if (reactor == NULL) return -1;
	memset(reactor, 0, sizeof(struct greactor));

	reactor->epfd = epoll_create(1);
	if (reactor->epfd <= 0) {
		printf("create epfd in %s err %s\n", __func__, strerror(errno));
		return -2;
	}

	reactor->events = (struct gevent*)malloc((MAX_EPOLL_EVENTS) * sizeof(struct gevent));
	if (reactor->events == NULL) {
		printf("create epfd in %s err %s\n", __func__, strerror(errno));
		close(reactor->epfd);
		return -3;
	}
}

int greactor_destory(struct greactor *reactor) {

	close(reactor->epfd);
	free(reactor->events);

}



int greactor_addlistener(struct greactor *reactor, int sockfd, GCALLBACK *acceptor) {

	if (reactor == NULL) return -1;
	if (reactor->events == NULL) return -1;

	gevent_set(&reactor->events[sockfd], sockfd, acceptor, reactor);
	gevent_add(reactor->epfd, EPOLLIN, &reactor->events[sockfd]);

	return 0;
}



int greactor_run(struct greactor *reactor) {
	if (reactor == NULL) return -1;
	if (reactor->epfd < 0) return -1;
	if (reactor->events == NULL) return -1;
	
	struct epoll_event events[MAX_EPOLL_EVENTS+1];
	
	int checkpos = 0, i;

	while (1) {
#if 0
		long now = time(NULL);
		for (i = 0;i < 100;i ++, checkpos ++) {
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
#endif

		int nready = epoll_wait(reactor->epfd, events, MAX_EPOLL_ITEM, 1000);
		if (nready < 0) {
			printf("epoll_wait error, exit\n");
			continue;
		}

		for (i = 0;i < nready;i ++) {

			struct gevent *ev = (struct gevent*)events[i].data.ptr;

			if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
				ev->callback(ev->fd, events[i].events, ev->arg);
			}
			if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
				ev->callback(ev->fd, events[i].events, ev->arg);
			}
			
		}

	}
}

int main(int argc, char **argv) {

	unsigned short port = SERVER_PORT;
	if (argc == 2) {
		port = atoi(argv[1]);
	}

	int listenfds[LISTEN_PORT_COUNT]={0};

	struct greactor *reactor = (struct greactor*)malloc(sizeof(struct greactor));
	greactor_init(reactor);

	for (size_t i = 0; i < LISTEN_PORT_COUNT; i++){
	    listenfds[i]= init_sock(port+i);
		greactor_addlistener(reactor, listenfds[i], accept_callback);
	}
	

	


	
	
	greactor_run(reactor);

	greactor_destory(reactor);

	for (size_t i = 0; i < LISTEN_PORT_COUNT ; i++)	{
	  close(listenfds[i]);
	}
	

	

	return 0;
}



