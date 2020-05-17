# UDP的连接性和负载均衡

## UDP的”连接性”好处

UDP的连接性可以带来以下2个好处。

### 高效率、低消耗
```cpp
int connect(int socket, const struct sockaddr *address, socklen_t address_len);             
ssize_t send(int socket, const void *buffer, size_t length,  int flags);
ssize_t sendto(int socket, const void *message,  size_t length,  int flags, const struct sockaddr *dest_addr,  socklen_t dest_len);
ssize_t recv(int socket, void *buffer, size_t length, int flags);
ssize_t recvfrom(int socket, void *restrict buffer,  size_t length,  int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);
```
细看两个系统调用的参数便知道，sendto比send的参数多2个，这就意味着每次系统调用都要多拷贝一些数据到内核空间 

同时，参数到内核空间后，内核还需要初始化一些临时的数据结构来存储这些参数值(主要是对端Endpoint_S的地址信息)，在数据包发出去后，内核还需要在合适的时候释放这些临时的数据结构。


进行UDP通信的时候，如果首先调用connect绑定对端Endpoint_S的后，那么就可以直接调用send来给对端Endpoint_S发送UDP数据包了。
用户在connect之后，内核会永久维护一个存储对端Endpoint_S的地址信息的数据结构，内核不再需要分配/删除这些数据结构，只需要查找就可以了，从而减少了数据的拷贝。这

### 错误提示

相信大家写 UDP Socket 程序的时候，有时候在第一次调用 sendto 给一个 unconnected UDP socket 发送 UDP 数据包时，接下来调用 recvfrom() 或继续调sendto的时候会返回一个 ECONNREFUSED 错误。

对于一个无连接的 UDP 是不会返回这个错误的，之所以会返回这个错误，是因为你明确调用了 connect 去连接远端的 Endpoint_S 了。那么这个错误是怎么产生的呢？没有调用 connect 的 UDP Socket 为什么无法返回这个错误呢？

当一个 UDP socket 去 connect 一个远端 Endpoint_S 时，并没有发送任何的数据包，其效果仅仅是在本地建立了一个五元组映射，对应到一个对端，该映射的作用正是为了和 UDP 带外的 ICMP 控制通道捆绑在一起，使得 UDP socket 的接口含义更加丰满。这样内核协议栈就维护了一个从源到目的地的单向连接，当下层有ICMP(对于非IP协议，可以是其它机制)错误信息返回时，内核协议栈就能够准确知道该错误是由哪个用户socket产生的，这样就能准确将错误转发给上层应用了。对于下层是IP协议的时候，ICMP 错误信息返回时，ICMP 的包内容就是出错的那个原始数据包，根据这个原始数据包可以找出一个五元组，根据该五元组就可以对应到一个本地的connect过的UDP socket，进而把错误消息传输给该 socket，应用程序在调用socket接口函数的时候，就可以得到该错误消息了。

对于一个无“连接”的UDP，sendto系统调用后，内核在将数据包发送出去后，就释放了存储对端Endpoint_S的地址等信息的数据结构了，这样在下层的协议有错误返回的时候，内核已经无法追踪到源socket了。

这里有个注意点要说明一下，由于UDP和下层协议都是不可靠的协议，所以，不能总是指望能够收到远端回复的ICMP包，例如：中间的一个节点或本机禁掉了ICMP，socket api调用就无法捕获这些错误了。

## UDP的负载均衡 

TCP服务器大多采用accept/fork模式，TCP服务的MPM机制(multi processing module)，不管是预先建立进程池，还是每到一个连接创建新线程/进程，总体都是源于accept/fork的变体 

然而对于UDP却无法很好的采用PMP机制，由于UDP的无连接性、无序性，它没有通信对端的信息，不知道一个数据包的前置和后续，它没有很好的办法知道，还有没后续的数据包以及如果有的话，过多久才会来，会来多久，因此UDP无法为其预先分配资源。

### 端口重用：SO_REUSEADDR、SO_REUSEPORT

UDP的无连接状态(没有已有对端的信息)，使得UDP没有一个有效的办法来判断四元组是否冲突，于是对于新来的请求，UDP无法进行资源的预分配，于是多处理模式难以进行，最终只能“守株待兔“，

### UDP Socket列表变化问题

### UDP和Epoll结合：UDP的Accept模型

### UDP Fork 模型：UDP accept模型之按需建立UDP处理进程


### 如何让不可靠的UDP变的可靠

## UDP的三角制约原则

## 实时通信中什么是“可靠”

## UDP 为什么要可靠

## 重传模式

## RTT 与 RTO 的计算

## 窗口与拥塞控制

## 传输路径