---
layout: post
title: 'LT和ET解读'
subtitle: ''
date: 2019-09-18
categories: 技术
cover: 'http://on2171g4d.bkt.clouddn.com/jekyll-theme-h2o-postcover.jpg'
tags: 网络编程 
---

## epoll接口
```
int epoll_create(int size); //成功时返回epoll文件描述符，失败时返回-1
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event); //成功时返回0，失败时返回-1
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout); //成功时返回发生事件的文件描述数，失败时返回-1

```
动作，用三个宏来表示：
```
EPOLL_CTL_ADD：注册新的fd到epfd中；

EPOLL_CTL_MOD：修改已经注册的fd的监听事件；

EPOLL_CTL_DEL：从epfd中删除一个fd；
```

struct epoll_event结构如下：
```
typedef union epoll_data {
    void *ptr;
    int fd;
    __uint32_t u32;
    __uint64_t u64;
} epoll_data_t;

struct epoll_event {
    __uint32_t events; /* Epoll events */
    epoll_data_t data; /* User data variable */
};
```
events可以是以下几个宏的集合：

```
EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
EPOLLOUT：表示对应的文件描述符可以写；
EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
EPOLLERR：表示对应的文件描述符发生错误；
EPOLLHUP：表示对应的文件描述符被挂断；
EPOLLET ：将EPOLL设为边缘触发(Edge Triggered)模式
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
```

## ET和LT的触发方式
Level Triggered (LT) 水平触发
>socket接收缓冲区不为空 有数据可读 读事件一直  
>socket发送缓冲区不满 可以继续写入数据 写事件一直触发


Edge Triggered (ET) 边沿触发
> socket的接收缓冲区状态变化时触发读事件，即空的接收缓冲区刚接收到数据时触发读事件  
> socket的发送缓冲区状态变化时触发写事件，即满的缓冲区刚空出空间时触发读事件 仅在状态变化时触发事件

## ET和LT的处理过程
LT的处理过程：
* accept一个连接，添加到epoll中监听EPOLLIN事件
* 当EPOLLIN事件到达时，read fd中的数据并处理
* 当需要写出数据时，把数据write到fd中；如果数据较大，无法一次性写出，那么在epoll中监听EPOLLOUT事件
* 当EPOLLOUT事件到达时，继续把数据write到fd中；如果数据写出完毕，那么在epoll中关闭EPOLLOUT事件

ET的处理过程：
* accept一个连接，添加到epoll中监听EPOLLIN|EPOLLOUT事件
* 当EPOLLIN事件到达时，read fd中的数据并处理，read需要一直读，直到返回EAGAIN为止
* 当需要写出数据时，把数据write到fd中，直到数据全部写完，或者write返回EAGAIN
* 当EPOLLOUT事件到达时，继续把数据write到fd中，直到数据全部写完，或者write返回EAGAIN

从ET的处理过程中可以看到，ET的要求是需要一直读写，直到返回EAGAIN，否则就会遗漏事件。  
而LT的处理过程中，直到返回EAGAIN不是硬性要求，但通常的处理过程都会读写直到返回EAGAIN，但LT比ET多了一个开关EPOLLOUT事件的步骤




## 代码

给出[epoll-LT模型的使用](https://gitee.com/shixianguo/glearn/blob/master/select_poll_epoll/server_epoll_lt.cxx)

给出[epoll-ET模型的使用](https://gitee.com/shixianguo/glearn/blob/master/select_poll_epoll/server_epoll_et.cxx)



accept的时候区别 