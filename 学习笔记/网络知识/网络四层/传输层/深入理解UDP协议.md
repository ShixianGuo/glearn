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
sendto比send的参数多2个，这就意味着每次系统调用都要多拷贝一些数据到内核空间 

* 参数到内核空间后，内核还需要初始化一些临时的数据结构来存储这些参数值
* 在数据包发出去后，内核还需要在合适的时候释放这些临时的数据结构。


如果首先调用connect绑定对端Endpoint_S的后，那么可以直接调用send来给对端Endpoint_S发送UDP数据包了。
* 用户在connect之后，内核会永久维护一个存储对端Endpoint_S的地址信息结构，
* 内核不再需要分配/删除这些数据结构，只需要查找就可以了，减少了数据的拷贝。

### 错误提示

相信大家写 UDP Socket 程序的时候，有时候在第一次调用 sendto 给一个 unconnected UDP socket 发送 UDP 数据包时，接下来调用 recvfrom() 或继续调sendto的时候会返回一个 ECONNREFUSED 错误。

对于一个无连接的 UDP 是不会返回这个错误的，之所以会返回这个错误，是因为你明确调用了 connect 去连接远端的 Endpoint_S 了。那么这个错误是怎么产生的呢？

* 当一个 UDP socket 去 connect 一个远端 Endpoint_S 时，并没有发送任何的数据包，其效果仅仅是在本地建立了一个五元组映射，对应到一个对端，该映射的作用正是为了和 UDP 带外的 ICMP 控制通道捆绑在一起，使得 UDP socket 的接口含义更加丰满。这样内核协议栈就维护了一个从源到目的地的单向连接，当下层有ICMP(对于非IP协议，可以是其它机制)错误信息返回时，内核协议栈就能够准确知道该错误是由哪个用户socket产生的，这样就能准确将错误转发给上层应用了。对于下层是IP协议的时候，ICMP 错误信息返回时，ICMP 的包内容就是出错的那个原始数据包，根据这个原始数据包可以找出一个五元组，根据该五元组就可以对应到一个本地的connect过的UDP socket，进而把错误消息传输给该 socket，应用程序在调用socket接口函数的时候，就可以得到该错误消息了。

没有调用 connect 的 UDP Socket 为什么无法返回这个错误呢？

* 对于一个无“连接”的UDP，sendto系统调用后，内核在将数据包发送出去后，就释放了存储对端Endpoint_S的地址等信息的数据结构了，这样在下层的协议有错误返回的时候，内核已经无法追踪到源socket了。

这里有个注意点要说明一下，由于UDP和下层协议都是不可靠的协议，所以，不能总是指望能够收到远端回复的ICMP包，例如：中间的一个节点或本机禁掉了ICMP，socket api调用就无法捕获这些错误了。

## UDP的负载均衡 

* TCP服务器大多采用accept/fork模式，TCP服务的MPM机制(multi processing module)，不管是预先建立进程池，还是每到一个连接创建新线程/进程，总体都是源于accept/fork的变体 

* 然而对于UDP却无法很好的采用MPM机制，由于UDP的无连接性、无序性，它没有通信对端的信息，不知道一个数据包的前置和后续，它没有很好的办法知道，还有没后续的数据包以及如果有的话，过多久才会来，会来多久，因此UDP无法为其预先分配资源。

### 端口重用：SO_REUSEADDR、SO_REUSEPORT

UDP的无连接状态(没有已有对端的信息)，使得UDP没有一个有效的办法来判断四元组是否冲突，于是对于新来的请求，UDP无法进行资源的预分配，于是多处理模式难以进行，最终只能“守株待兔“
* UDP按照固定的算法查找目标UDP socket，这样每次查到的都是UDP socket列表固定位置的socket。
* UDP只是简单基于目的IP和目的端口来进行查找，这样在一个服务器上多个进程内创建多个绑定相同IP地址(SO_REUSEADDR)，相同端口的UDP socket，那么你会发现，**只有最后一个创建的socket会接收到数据，其它的都是默默地等待**，孤独地等待永远也收不到UDP数据。

要实现多处理，那么就要改变UDP Socket查找的考虑因素
* 对于调用了connect的UDP Client而言，由于其具有了“连接”性，通信双方都固定下来了，那么内核就可以根据4元组完全匹配的原则来匹配。
* 对于server端，在使用`SO_REUSEPORT`选项(linux 3.9以上内核)，这样在进行UDP socket查找的时候，源IP地址和源端口也参与进来了

### UDP Socket列表变化问题

通过上面我们知道，在采用SO_REUSEADDR、SO_REUSEPORT这两个socket选项后
* 内核会根据UDP数据包的4元组来查找本机上的所有相同目的IP地址，相同目的端口的socket中的一个socket的位置，然后以这个位置上的socket作为接收数据的socket。

要确保来至同一个Client Endpoint的UDP数据包总是被同一个socket来处理，就需要保证整个socket链表的socket所处的位置不能改变 
* **然而，如果socket链表中间的某个socket挂了的话，就会造成socket链表重新排序，这样会引发问题。**

于是基本的解决方案是
* 在整个服务过程中不能关闭UDP socket(当然也可以全部UDP socket都close掉，从新创建一批新的)。
    * 要保证这一点，我们需要所有的UDP socket的创建和关闭都由一个master进行来管理，worker进程只是负责处理对于的网络IO任务
    * 为此我们需要socket在创建的时候要带有CLOEXEC标志(SOCK_CLOEXEC)。


### UDP和epoll结合：UDP的Accept模型

为了充分利用多核CPU资源，进行UDP的多处理，我们会预先创建多个进程，
每个进程都创建一个或多个绑定相同端口，相同IP地址(SO_REUSEADDR、SO_REUSEPORT)的UDP socket 
这样利用内核的UDP socket查找算法来达到UDP的多进程负载均衡.

* 然而，这完全依赖于Linux内核处理UDP socket查找时的一个算法，我们不能保证其它的系统或者未来的Linux内核不会改变算法的行为；


UDP svr无法充分利用epoll的高性能event机制的主要原因
* UDP svr只有一个UDP socket来接收和响应所有client的请求。
 
**然而如果能够为每个client都创建一个socket并虚拟一个“连接”与之对应**，这样不就可以充分利用内核UDP层的socket查找结果和epoll的通知机制了么。

**server端具体过程如下**
* [1] UDP svr创建UDP socket fd,设置socket为REUSEADDR和REUSEPORT、同时bind本地地址local_addr：
* [2] 创建epoll fd，并将listen_fd放到epoll中 并监听其可读事件：
* [3] epoll_wait返回时，
    * 如果epoll_wait返回的事件fd是listen_fd，
    * 调用recvfrom接收client第一个UDP包并根据recvfrom返回的client地址, 
    * 创建一个新的socket(new_fd)与之对应，
    * 设置new_fd为REUSEADDR和REUSEPORT、同时bind本地地址local_addr，
    * 然后connect上recvfrom返回的client地址：
* [4] 将新创建的new_fd加入到epoll中并监听其可读等事件：
* [5] 当epoll_wait返回时，
    * 如果epoll_wait返回的事件fd是new_fd 
    * 那么就可以调用recvfrom来接收特定client的UDP包了：

这里的UPD和Epoll结合方案，有以下几个注意点：
* [1] client要使用固定的ip和端口和server端通信，也就是client需要bind本地local address：
* [2] 要小心处理上面步骤3中connect返回前，Client已经有多个UDP包到达Server端的情况：
     * 如果server没处理好这个情况，在connect返回前，有2个UDP包到达server端了，
     * 这样server会new出两个new_fd1和new_fd2分别connect到client，
     * 那么后续的client的UDP到达server的时候，内核会投递UDP包给new_fd1和new_fd2中的一个。

上面的UDP和Epoll结合的accept模型有个不好处理的小尾巴(也就是上面的注意点[2])，这个小尾巴的存在其本质是UDP和4元组没有必然的对应关系，也就是UDP的无连接性。

### UDP fork 模型：UDP accept模型之按需建立UDP处理进程


为了充分利用多核 CPU (为简化讨论，不妨假设为8核)，理想情况下，同时有8个工作进程在同时工作处理请求。于是我们会初始化8个绑定相同端口，相同IP地址(SO_REUSEADDR、SO_REUSEPORT)的 UDP socket ，接下来就靠内核的查找算法来达到client请求的负载均衡了。
* 由于内核查找算法是固定的，于是，无形中所有的client被划分为8类，
    * 类型1的所有client请求全部被路由到工作进程1的UDP socket由工作进程1来处理，同样类型2的client的请求也全部被工作进程2来处理。
* 这样的缺陷是明显的，比较容易造成短时间的负载极端不均衡。

一般情况下，如果一个 UDP 包能够标识一个请求，
* 那么简单的解决方案是每个 UDP socket n 的工作进程 n，自行 fork 出多个子进程来处理类型n的 client 的请求。
* 这样每个子进程都直接 recvfrom 就 OK 了，拿到 UDP 请求包就处理，拿不到就阻塞。

如果一个请求需要多个 UDP 包来标识的情况下，事情就没那么简单了,我们需要将同一个 client 的所有 UDP 包都路由到同一个工作子进程。
* 我们需要一个master进程来监听UDP socket的可读事件，master进程监听到可读事件，
* 就采用MSG_PEEK选项来recvfrom数据包，如果发现是新的Endpoit(ip、port)Client的UDP包，那么就fork一个新的进行来处理该Endpoit的请求。

具体如下：
* [1] master进程监听udp_socket_fd的可读事件：
    * 当可读事件到来 
       * 探测一下到来的UDP包是否是新的client的UDP包
       * 查找一下worker_list是否为该client创建过worker进程了
* [2] 如果没有查找到，就fork()处理进程来处理该请求，并将该client信息记录到worker_list中。
     * 查找到，那么continue，回到步骤[1]。
* [3] 每个worker子进程，保存自己需要处理的client信息pclientaddr

该fork模型很别扭，过多的探测行为，
* 一个数据包来了，会”惊群”唤醒所有worker子进程，大家都去PEEK一把，最后只有一个worker进程能够取出UDP包来处理。
* 同时到来的数据包只能排队被取出。
* 更为严重的是，由于recvfrom的排他唤醒，可能会造成死锁



# 如何让不可靠的UDP变的可靠

## UDP的三角制约原则

<div align="center"> <img src="pic/udp01.png"/> </div>

RUDP 的价值在于根据不同的传输场景进行不同的技术选型，可能选择宽松的拥塞方式、也可能选择特定的重传模式，但不管怎么选，都是基于 expense(成本)、latency（时延）、quality（质量）三者之间来权衡，通过结合场景和权衡三角平衡关系 RUDP 或许能帮助开发者找到一个比较好的方案。

## 实时通信中什么是“可靠”
在实时通信过程中，不同的需求场景对可靠的需求是不一样的，我们在这里总体归纳为三类定义

* 尽力可靠：通信的接收方要求发送方的数据尽量完整到达，但业务本身的数据是可以允许缺失的。例如：音视频数据、幂等性状态数据。；
* 无序可靠：通信的接收方要求发送方的数据必须完整到达，但可以不管到达先后顺序。例如：文件传输、白板书写、图形实时绘制数据、日志型追加数据等；
* 有序可靠：通信接收方要求发送方的数据必须按顺序完整到达。
RUDP 是根据这三类需求和上节图中的三角制约关系来确定自己的通信模型和机制的，也就是找通信的平衡点。

## UDP 为什么要可靠
RUDP 主要解决以下相关问题
* 端对端连通性问题：
* 弱网环境传输问题：
* 带宽竞争问题
* 传输路径优化问题：
* 资源优化问题：


## 重传模式
IP 协议在设计的时候就不是为了数据可靠到达而设计的，所以 UDP 要保证可靠，就依赖于重传，这也就是我们通常意义上的 RUDP 行为。

<div align="center"> <img src="pic/udp02.png"/> </div>

RUDP 的重传是发送端通过接收端 ACK 的丢包信息反馈来进行数据重传，发送端会根据场景来设计自己的重传方式，重传方式分为三类：定时重传、请求重传和 FEC 选择重传。

### 定时重传
这种方式依赖于接收端的 ACK 和 RTO，容易产生误判，主要有两种情况：
* 1）对方收到了数据包，但是 ACK 发送途中丢失；
* 2）ACK 在途中，但是发送端的时间已经超过了一个 RTO。

所以超时重传的方式主要集中在 RTO 的计算上，如果你的场景是一个对延迟敏感但对流量成本要求不高的场景，就可以将 RTO 的计算设计得比较小，这样能尽最大可能保证你的延时足够小

例如：实时操作类网游、教育领域的书写同步，是典型的用 expense 换 latency 和 quality 的场景，适合用于小带宽低延迟传输。如果是大带宽实时传输，定时重传对带宽的消耗是很大的，极端情况会有 20% 的重传率，所以在大带宽模式下一般会采用请求重传模式。

### 请求重传
请求重传就是接收端在发送 ACK 的时候携带自己丢失报文的信息反馈，发送端接收到 ACK 信息时根据丢包反馈进行报文重传
<div align="center"> <img src="pic/udp03.png"/> </div>

关键的步骤就是回送 ACK 的时候应该携带哪些丢失报文的信息，
* 因为 UDP 在网络传输过程中会乱序会抖动，接收端在通信的过程中要评估网络的 jitter time，也就是 rtt_var（RTT 方差值），当发现丢包的时候记录一个时刻 t1，当 t1 + rtt_var < curr_t(当前时刻)，我们就认为它丢失了。

* 这个时候后续的 ACK 就需要携带这个丢包信息并更新丢包时刻 t2，后续持续扫描丢包队列，如果 t2 + RTO <curr_t，则再次在 ACK 携带这个丢包信息，以此类推，直到收到报文为止。

整个请求重传机制依赖于 jitter time 和 RTO 这个两个时间参数，评估和调整这两个参数和对应的传输场景也息息相关。请求重传这种方式比定时重传方式的延迟会大，一般适合于带宽较大的传输场景，例如：视频、文件传输、数据同步等。

### FEC 选择重传
除了定时重传和请求重传模式以外，还有一种方式就是以 FEC 分组方式选择重传，FEC（Forward Error Correction）是一种前向纠错技术，一般通过 XOR 类似的算法来实现，也有多层的 EC 算法和 raptor 涌泉码技术，其实是一个解方程的过程。

<div align="center"> <img src="pic/udp04.png"/> </div>

在发送方发送报文的时候，会根据 FEC 方式把几个报文进行 FEC 分组，通过 XOR 的方式得到若干个冗余包，然后一起发往接收端，如果接收端发现丢包但能通过 FEC 分组算法还原，就不向发送端请求重传，如果分组内包是不能进行 FEC 恢复的，就向发送端请求原始的数据



## RTT 与 RTO 的计算

在上面介绍重传模式时多次提到 RTT、RTO 等时间度量，RTT（Round Trip Time）即网络环路延时，环路延迟是通过发送的数据包和接收到的 ACK 包计算的，示意图如下：

<div align="center"> <img src="pic/udp05.png"/> </div>

**RTT = T2 - T1**

**SRTT = (α * SRTT) + (1-α)RTT：**

**RTO = SRTT + SRTT_VAR**

RTO 就是一个报文的重传周期
RTT 往返时延

(主机从发出数据包到第一次TCP重传开始，这段时间间隔称为retransmission timeout，缩写做RTO)



## 窗口与拥塞控制

### 窗口
RUDP 需要一个收发的滑动窗口系统来配合对应的拥塞算法做流量控制，
* 有些 RUDP 需要发送端和接收端的窗口严格地对应，
* 有些 RUDP 不要求收发窗口严格对应。
* 如果涉及到可靠有序的 RUDP，接收端就要做窗口排序和缓冲，
* 如果是无序可靠或者尽力可靠的场景，接收端一般就不做窗口缓冲，只做位置滑动。

<div align="center"> <img src="pic/udp06.png"/> </div>

### 经典拥塞算法
TCP 经典拥塞算法分为四个部分：慢启动、拥塞避免、拥塞处理和快速恢复
经典算法是建立在定时重传的基础上的，如果 RUDP 采用这种算法来做拥塞控制，一般的场景是为了保证有序可靠传输的同时又兼顾网络传输的公平性原则

### BBR 拥塞算法

对于经典拥塞算法的延迟和带宽压榨问题 Google 设计了基于发送端延迟和带宽评估的 BBR 拥塞控制算法。

这种拥塞算法致力于解决两个问题：
* 1）在一定丢包率网络传输链路上充分利用带宽；
* 2）降低网络传输中的 buffer 延迟。

**BBR 的主要策略是周期性通过 ACK 和 NACK 返回来评估链路的 min_rtt 和 max_bandwidth。**最大吞吐量（cwnd）的大小就是：cwnd = max_bandwidth / min_rtt。

<div align="center"> <img src="pic/udp07.png"/> </div>


BBR 适合在随机丢包且网络稳定的情况下做拥塞，如果在网络信号极不稳定的 Wi-Fi 或者 4G 上，容易出现网络泛洪和预测不准的问题，BBR 在多连接公平性上也存在小 RTT 的连接比大 RTT 的连接更吃带宽的情况，容易造成大 RTT 的连接速度过慢的情况。BBR 拥塞算法在三角平衡关系中是采用 expense 换取 latency 和 quality 的案例。


### WebRTC GCC
WebRTC 的 GCC 是一个基于发送端丢包率和接收端延迟带宽统计的拥塞控制，而且是一个尽力可靠的传输算法，在传输的过程中如果一个报文重发太多次后会直接丢弃，这符合视频传输的场景
<div align="center"> <img src="pic/udp08.png"/> </div>


但这种算法也有个缺陷，就是在网络间歇性丢包情况下，GCC 可能收敛的速度比较慢，在一定程度上有可能会造成 REMB 很难反馈给发送端，容易出现发送端流控失效。GCC 在三角平衡关系算一个以 quality 和 expense 换取 latency 的案例。

### 弱窗口拥塞机制

其实在很多场景是不用拥塞控制或者只要很弱的拥塞控制即可，例如：师生双方书写同步、实时游戏，因为本身的传输的数据量不大，只要保证足够小的延时和可靠性就行，一般会采用固定窗口大小来进行流控，我们在系统中一般采用一个 cwnd =32 这样的窗口来做流控，ACK 确认也是通过整个接收窗口数据状态反馈给发送方，简单直接，也很容易适应弱网环境。

## 传输路径

RUDP 除了优化连接、压榨带宽、适应弱网环境等以外，它也继承了 UDP 天然的动态性，可以在中间应用层链路上做传输优化，一般分为多点串联优化和多点并联优化。

### 多点串联 relay
在实时通信中一些业务场景对延迟非常敏感，例如：实时语音、同步书写、实时互动、直播连麦等，如果单纯的服务中转或者 P2P 通信，很难满足其需求，尤其是在物理距离很大的情况下。

在解决这个问题上 SKYPE 率先提出全球 RTN（实时多点传输网络），其实就是在通信双方之间通过几个 relay 节点来动态智能选路，这种传输方式很适合 RUDP，我们只要在通信双方构建一个 RUDP 通道，中间链路只是一个无状态的 relay cache 集合，relay 与 relay 之间进行路由探测和选路，以此来做到链路的高可用和实时性。

<div align="center"> <img src="pic/udp09.png"/> </div>


### 多点并联 relay
在服务与服务进行媒体数据传输或者分发过程中，需要保证传输路径高可用和带宽并发，这类使用场景也会使用传输双方构建一个 RUDP 通道，中间通过多 relay 节点的并联来解决。


<div align="center"> <img src="pic/udp10.png"/> </div>

这种模型需要在发送端设计一个多点路由表探测机制，以此来判断各个路径同时发送数据的比例和可用性，这个模型除了链路备份和增大传输并发带宽外，还有个辅助的功能，如果是流媒体分发系统，我们一般会用 BGP 来做中转，如果节点与节点之间可以直连，这样还可以减少对 BGP 带宽的占用，以此来减少成本。