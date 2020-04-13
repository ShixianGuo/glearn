首先看看一个典型互联网服务端处理网络请求的典型过程：
<div align="center"> <img src="pic/网络IO01.png"/> </div>

网络IO，会涉及到两个系统对象 ： 一个是用户空间调用 IO 的进程或者线程 ， 另一个是内核空间的内核系统。  
比如进行read操作的时候：会经历两个阶段
>* 等待数据准备就绪 
>* 将数据从内核拷贝到进程或者线程中。   

因为在以上两个阶段上各有不同的情况，所以出现了多种网络 IO 模型 

---
相关问题
>* ET和LT的区别？
>* 众多的网络IO改如何管理？管理什么？ 
>* 服务器如何知道众多网络IO中那些IO有数据可读？
---


## I/O模型1：阻塞式 I/O 模型(blocking I/O）
<div align="center"> <img src="pic/网络IO02.png"/> </div>

阻塞IO 的特点就是在 IO 执行的两个阶段（等待数据和拷贝数据两个阶段）都被 block 了  

实际上 大部分IO都是阻塞型的  这样导致的一个问题就是假如read的时候 整个线程将被阻塞挂起 无法进行其他任务运算  

一个简单的改进方案就是使用多线程的形式 既“**一请求一线程or进程**”这样好处就是一个任何一个连接阻塞不会影响其他的的收发。

这样做缺点还是灰常明显的，所以很多情况下，大部分人还是会考虑线程池和连接池  
 >“线程池”旨在减少创建和销毁线程 的频率，其维持一定合理数量的线程，并让空闲的线程重新承担新的执行任务。  
 >“连接池”维 持连接的缓存池，尽量重用已有的连接、减少创建和关闭连接的频率。

 
该模型下可以方便高效解决的小规模服务请求，但是大规模上来还是会遇到瓶颈 这个时候就要考虑其他方式了

## I/O模型2：非阻塞式 I/O 模型(non-blocking I/O）
<div align="center"> <img src="pic/网络IO03.png"/> </div>

从用户进程角度讲 ，它发起一个 read 操作后，并不需要等待，而是马上就得到了一个结果。
所以，在非阻塞式 IO 中，用户进程其实是需要不断的主动询问 kernel 数据准备好了没有。

在非阻塞状态下，recv() 接口在被调用后立即返回，返回值代表了不同的含义 
>* recv() 返回值大于 0，表示接受数据完毕，返回值即是接受到的字节数； 
>* recv() 返回 0，表示连接已经正常断开； 
>* recv() 返回 -1，且 errno 等于 EAGAIN，表示 recv 操作还没执行完成； 
>* recv() 返回 -1，且 errno 不等于 EAGAIN，表示 recv 操作遇到系统错误 errno。  


非阻塞的接口相比于阻塞型接口的显著差异在于，在被调用之后立即返回。使用如下 的函数可以将某句柄 fd 设为非阻塞状态。 

```c
fcntl( fd, F_SETFL, O_NONBLOCK );
```


## I/O模型3：I/O 复用模型(I/O multiplexing）
<div align="center"> <img src="pic/网络IO04.png"/> </div>

### select
这种方式也被成为事件驱动 它的基本原理就是 select/epoll 这个 function 会不断的轮询所负责的所有 socket，当某个 socket 有数据到达了，就通知用户进程。  


当用户进程调用了 select，那么整个进程会被 block，而同时，kernel 会
“监视”所 有 select 负责的 socket，当任何一个 socket 中的数据准备好了，select 就会返回。这 个时候用户进程再调用 read 操作，将数据从 kernel 拷贝到用户进程。

上图与阻塞IO不同的是，这里需 要使用两个系统调用(select 和 read)，而 阻塞 IO 只调用了一个系统调用(read)。  


但是使用 select 以后最大的优势是用户可以在一个线程内同时处理多个 socket 的 IO 请 求。用户可以注册多个 socket，即可达到 在同一个线程内同时处理多个 IO 请求的目的。而在同步阻塞模型中，必须通过多线程的方 式才能达到这个目的。  

在多路复用模型中，对于每一个 socket，一般都设置成为 non-blocking，但是，如 上图所示，整个用户的 process 其实是一直被 block 的。只不过 process 是被 select 这 个函数 block，而不是被 socket IO 给 block


select 接口的原型:  
```cpp
 FD_ZERO(int fd, fd_set* fds)      
 FD_SET(int fd, fd_set* fds)      
 FD_ISSET(int fd, fd_set* fds)      
 FD_CLR(int fd, fd_set* fds)     

 int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout)  

```

这种模型的特征在于每一个执行周期都会探测一次或一组事件，一个特定的事件会触发 某个特定的响应。我们可以将这种模型归类为“事件驱动模型”。 

select的特点
>* fd创建是依次往上增长  所以可以利用bitmap(fd_set)
>* 一旦某个fd有数据可读  就把bitmap[fd]置为1
>* 读写异常三个状态在不同的集合里面

但是select的缺点也很明显
>1-进程打开的fd有限（由FD_SETSIZE和内核决定1024)   
>2-fdset不可重用  
>3-用户态和内核态传递描述符结构时copy开销大  
>4-函数返回后需要轮询描述符集，寻找就绪描述符，效率不高

### poll
poll的特点
>* 把读写异常三个状态放到一个数组里面

poll的接口原型  
```  
int poll(struct pollfd fd[], nfds_t nfds, int timeout);  

struct pollfd{

　　int fd;           //文件描述符

　　short events;    //请求的事件

　　short revents;   //返回的事件

};
```

poll的优缺点
>1-因为是数组 解决了select 没有最大监视描述符数量的限制  
>2-不需要每次都重置fdset fdset可重用：把pollfd.revent置位  
>但没解决selcet的缺点三和四  
>3-用户态和内核态传递描述符结构时copy开销大  
>4-调用返回后需要轮询所有描述符来获取已经就绪的描述符


### epoll

epoll大概原理
```
执行epoll_create()时，创建了红黑树和就绪链表；
执行epoll_ctl()时
  如果增加socket句柄，则检查在红黑树中是否存在，存在立即返回，
  不存在则添加到树干上，然后向内核注册回调函数，用于当中断事件来临时向准备就绪链表中插入数据；
执行epoll_wait()时立刻返回准备就绪链表里的数据即可 链表没有就绪就会被阻塞挂起
```
epoll接口

```
int epoll_create(int size); //成功时返回epoll文件描述符，失败时返回-1
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event); //成功时返回0，失败时返回-1
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout); //成功时返回发生事件的文件描述数，失败时返回-1

```
op用三个宏来表示：
```
EPOLL_CTL_ADD：注册新的fd到epfd中；

EPOLL_CTL_MOD：修改已经注册的fd的监听事件；

EPOLL_CTL_DEL：从epfd中删除一个fd；
```

truct epoll_event结构如下：
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

//events可以是以下几个宏的集合：
EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
EPOLLOUT：表示对应的文件描述符可以写；
EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
EPOLLERR：表示对应的文件描述符发生错误；
EPOLLHUP：表示对应的文件描述符被挂断；
EPOLLET ：将EPOLL设为边缘触发模式
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
```

ET和LT的触发方式

Level Triggered (LT) 水平触发
>socket接收缓冲区不为空 有数据可读 读事件一直  
>socket发送缓冲区不满 可以继续写入数据 写事件一直触发


Edge Triggered (ET) 边沿触发
> socket的接收缓冲区状态变化时触发读事件，即空的接收缓冲区刚接收到数据时触发读事件  
> socket的发送缓冲区状态变化时触发写事件，即满的缓冲区刚空出空间时触发读事件 仅在状态变化时触发事件

## I/O模型4：信号驱动式 I/O 模型（signal-driven I/O)
<div align="center"> <img src="pic/网络IO05.png"/> </div>


首先我们允许套接口进行信号驱动 I/O,并安装一个信号处理函数，进程继续运行并不阻 塞。当数据准备好时，进程会收到一个 SIGIO 信号，可以在信号处理函数中调用 I/O 操作函 数处理数据

## I/O模型5：异步 I/O 模型（即AIO，全称asynchronous I/O）
<div align="center"> <img src="pic/网络IO06.png"/> </div>


## 5 种 I/O 模型总结
<div align="center"> <img src="pic/网络IO07.png"/> </div>

经过上面的叙述 非阻塞IO和异步IO还是有很大的区别的  

在非阻塞IO中，虽然进程大部分时间都不会被 block，但是它仍然要求进程去主动的 check， 并且当数据准备完成以后，也需要进程主动的再次调用recvfrom来将数据拷贝到用户内存  

而异步IO 则完全不同。它就像是用户进程将整个 IO 操作交给了他人（kernel）完成，然后他人做完后发信号通知。在此期间，用户进程不需要去检查 IO 操作的状态，也不 需要主动的去拷贝数据