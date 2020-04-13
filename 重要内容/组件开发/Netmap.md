## 1. Netmap 是啥 ？[链接](https://github.com/luigirizzo/netmap )

[数据结构](https://www.aikaiyuan.com/7711.html)

![](img/netmap结构.png)

```
在 Netmap 框架下，内核拥有数据包池，发送环接收环上的数据包不需要动态申请，有数据到达网卡时，当有数据到达后，直接从数据包池中取出一个数据包，然后将数据放入此数据包中，再将数据包的描述符放入接收环中。内核中的数据包池，通过 mmap 技术映射到用户 空间。用户态程序最终通过 netmap_if 获取接收发送环 netmap_ring，进行数据包的获取发 送。 
```

## 2. Netmap API 介绍 

###  简要说明
.netmap API 主要为两个头文件 netmap.h 和 netmap_user.h ，当解压下载好的 netmap 程序后，在./netmap/sys/net/目录下

系统通过一个四元组来唯一标识一条TCP连接： 本地IP  本地端口  远程IP  远程端口