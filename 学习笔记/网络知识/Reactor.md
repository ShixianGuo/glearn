## 前言
网络连接上的消息处理，可以分为两个阶段：**等待消息准备好、消息处理**。  

当使用默认的阻塞套接字时（一连接一线程），往往是把 这两个阶段合而为一这样操作套接字的代码所在的线程就得睡眠来等待消息准备好，这导致了线程会频繁的睡眠、唤醒，从而影响了 CPU 的使用效率。   

高并发编程方法当然就是把两个阶段分开处理。当然，这也要求套接字必须是非阻塞的，
   

作为一个高性能服务器程序通常需要考虑处理三类事件： **I/O 事件，定时事件及信号**

等待消息准备好：**就是IO多路复用了** 2333
两种高效的事件处理模型：**Reactor 和 Proactor。** 

**问题**
>* epoll是用来管理网络IO的 怎么管理大量的fd？

## Reactor 模型 
我们先看下普通函数的调用机制：
>程序调用某函数，函数执行，程序等待，函数将结果和控制权返回给程序，程序继续处理  

Reactor与普通函数调用机制的区别在于：
>应用程序不是主动的调用某个 API 完成处理，而是恰恰 相反，Reactor 逆置了事件处理流程，应用程序需要提供相应的接口并注册到 Reactor 上， 如果相应的时间发生，Reactor 将主动调用应用程序注册的接口，这些接口又称为“回调函数”。 

Reactor 中2个关键：
> 1）Reactor：Reactor 在一个单独的线程中运行，负责监听和分发事件，分发给适当的处理程序来对 IO 事件做出反应。 它就像公司的电话接线员，它接听来自客户的电话并将线路转移到适当的联系人；
> 2）Handlers：处理程序执行 I/O 事件要完成的实际事件，类似于客户想要与之交谈的公司中的实际官员。Reactor 通过调度适当的处理程序来响应 I/O 事件，处理程序执行非阻塞操作。

Reactor 三个重要组件： 
>* 多路复用器： epoll_wait
>* 事件分发器： if(fd&EPOLLIN)   if(fd&EPOLLOUT) 
>* 事件处理器： read_cb()        write_cb()  

具体流程如下
>1. 注册读就绪事件和相应的事件处理器  
>2. 事件分离器等待事件； 
>3. 事件到来，激活分离器，分离器调用事件对应的处理器； 
>4. 事件处理器完成实际的读操作，处理读到的数据，注册新的事件，然后返还控制 权

Reactor优点： 
> 响应快，不必为单个同步时间所阻塞，虽然 Reactor 本身依然是同步的  
> 编程相对简单
> 可扩展性，可以方便的通过增加 Reactor 实例个数来充分利用 CPU 资源  
> 可复用性，reactor框架本身与具体事件处理逻辑无关，具有很高的复用性； 

Reactor 模型开发效率上比起直接使用IO复用要高，它通常是单线程的，设计目标是希望单线程使用一颗CPU的全部资源，但也有附带优点，即每个事件处理中很多时候可以 不考虑共享资源的互斥访问  

但是缺点也是明显的  多核场景下就有点弱鸡和悲剧了。但是并不是所有的多核场景都这样 主要还是看业务的相关性 例如 如果程序业务很简单，例如只是简单的访问一些提供了并发访问的服务，就可以直接开 启多个反应堆，每个反应堆对应一颗 CPU 核心，这些反应堆上跑的请求互不相关，这是完全可以利用多核的


根据 Reactor 的数量和处理资源池线程的数量不同，有 3 种典型的实现
1）单 Reactor 单线程； 
 <div align="center"> <img src="pic/Reactor01.png"/> </div>

方案说明：

* 1）Reactor 对象通过 Select 监控客户端请求事件，收到事件后通过 Dispatch 进行分发；  
* 2）如果是建立连接请求事件，则由 Acceptor 通过 Accept 处理连接请求，然后创建一个 Handler 对象处理连接完成后的后续业务处理；  
* 3）如果不是建立连接事件，则 Reactor 会分发调用连接对应的 Handler 来响应；  
* 4）Handler 会完成 Read→业务处理→Send 的完整业务流程。


使用场景：客户端的数量有限，业务处理非常快速，比如 Redis，业务处理的时间复杂度 O(1)。

2）单 Reactor 多线程；  
<div align="center"> <img src="pic/Reactor02.png"/> </div>

方案说明:  

* 1）Reactor 对象通过 Select 监控客户端请求事件，收到事件后通过 Dispatch 进行分发；  
* 2）如果是建立连接请求事件，则由 Acceptor 通过 Accept 处理连接请求，然后创建一个 Handler 对象处理连接完成后续的各种事件；  
* 3）如果不是建立连接事件，则 Reactor 会分发调用连接对应的 Handler 来响应；  
* 4）Handler 只负责响应事件，不做具体业务处理，通过 Read 读取数据后，会分发给后面的 Worker 线程池进行业务处理；  
* 5）Worker 线程池会分配独立的线程完成真正的业务处理，如何将响应结果发给 Handler 进行处理；  
* 6）Handler 收到响应结果后通过 Send 将响应结果返回给 Client。



3）主从 Reactor 多线程;

<div align="center"> <img src="pic/Reactor03.png"/> </div>

针对单 Reactor 多线程模型中，Reactor 在单线程中运行，高并发场景下容易成为性能瓶颈，可以让 Reactor 在多线程中运行。

方案说明：
* 1）Reactor 主线程 MainReactor 对象通过 Select 监控建立连接事件，收到事件后通过 Acceptor 接收，处理建立连接事件；
* 2）Acceptor 处理建立连接事件后，MainReactor 将连接分配 Reactor 子线程给 SubReactor 进行处理；
* 3）SubReactor 将连接加入连接队列进行监听，并创建一个 Handler 用于处理各种连接事件；
* 4）当有新的事件发生时，SubReactor 会调用连接对应的 Handler 进行响应；
* 5）Handler 通过 Read 读取数据后，会分发给后面的 Worker 线程池进行业务处理；
* 6）Worker 线程池会分配独立的线程完成真正的业务处理，如何将响应结果发给 Handler 进行处理；
* 7）Handler 收到响应结果后通过 Send 将响应结果返回给 Client。

优点：
* 父线程与子线程的数据交互简单职责明确，父线程只需要接收新连接，子线程完成后续的业务处理。
* 父线程与子线程的数据交互简单，Reactor 主线程只需要把新连接传给子线程，子线程无需返回数据。

这种模型在许多项目中广泛使用，包括 Nginx 主从 Reactor 多进程模型，Memcached 主从多线程，Netty 主从多线程模型的支持。


## Proactor 模型 

在 Reactor 模式中，Reactor 等待某个事件或者可应用或者操作的状态发生（比如文件描述符可读写，或者是 Socket 可读写）。

然后把这个事件传给事先注册的 Handler（事件处理函数或者回调函数），由后者来做实际的读写操作。

其中的读写操作都需要应用程序同步操作，所以 Reactor 是非阻塞同步网络模型。

如果把 I/O 操作改为异步，即交给操作系统来完成就能进一步提升性能，这就是异步网络模型 Proactor。

<div align="center"> <img src="pic/Proactor.png"/> </div>

Proactor 是和异步 I/O 相关的，详细方案如下：

* 1）Proactor Initiator 创建 Proactor 和 Handler 对象，并将 Proactor 和 Handler 都通过 AsyOptProcessor（Asynchronous Operation Processor）注册到内核；
* 2）AsyOptProcessor 处理注册请求，并处理 I/O 操作；
* 3）AsyOptProcessor 完成 I/O 操作后通知 Proactor；
* 4）Proactor 根据不同的事件类型回调不同的 Handler 进行业务处理；
* 5）Handler 完成业务处理。

可以看出 Proactor 和 Reactor 的区别：

* 1）Reactor 是在事件发生时就通知事先注册的事件（读写在应用程序线程中处理完成）；
* 2）Proactor 是在事件发生时基于异步 I/O 完成读写操作（由内核完成），待 I/O 操作完成后才回调应用程序的处理器来进行业务处理。

 
Proactor 增加了编程的复杂度，但给工作线程带来了更高的效率。Proactor 可以在 系统态将读写优化，利用 I/O 并行能力，提供一个高性能单线程模型。在 windows 上， 由于没有 epoll 这样的机制，因此提供了 IOCP 来支持高并发， 由于操作系统做了较好的 优化，windows 较常采用 Proactor 的模型利用完成端口来实现服务器。在 linux 上，在 2.6 内核出现了 aio 接口，但 aio 实际效果并不理想，它的出现主要是解决 poll 性能不佳的问题，但实际上经过测试，epoll 的性能高于 poll+aio，并且aio 不能处理 accept， 因此 linux 主要还是以 Reactor 模型为主。   


## 同步I/O 模拟 Proactor 模型 

<div align="center"> <img src="pic/同步IO模拟Proactor.png"/> </div>

详细方案如下

1. 主线程往 epoll 内核事件表中注册 socket 上的读就绪事件。 
2. 主线程调用 epoll_wait 等待 socket 上有数据可读。 
3. 当 socket 上有数据可读时，epoll_wait 通知主线程。主线程从 socket 循环读取数据， 直到没有更多数据可读，然后将读取到的数据封装成一个请求对象并插入请求队列。
4. 睡眠在请求队列上的某个工作线程被唤醒，它获得请求对象并处理客户请求，然后 往 epoll 内核事件表中注册 socket 上的写就绪事件。 、
5. 主线程调用 epoll_wait 等待 socket 可写。 
6. 当 socket 可写时，epoll_wait 通知主线程。主线程往 socket 上写入服务器处理客户 请求的结果。


两个模式的相同点，都是对某个 IO 事件的事件通知(即告诉某个模块，这个 IO 操作可 以进行或已经完成)。在结构上两者也有相同点：demultiplexor 负责提交 IO 操作(异步)、 查询设备是否可操作(同步)，然后当条件满足时，就回调注册处理函数。   

不同点在于，异步情况下(Proactor)，当回调注册的处理函数时，表示 IO 操作已经完 成；同步情况下(Reactor)，回调注册的处理函数时，表示 IO 设备可以进行某个操作(can read or can write)，注册的处理函数这个时候开始提交操作。 