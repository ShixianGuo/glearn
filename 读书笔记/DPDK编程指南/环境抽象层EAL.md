# 环境抽象层

## 什么是EAL？ 

EAL的全称是(Environment Abstraction Layer, 环境抽象层)，它负责为应用间接访问底层的资源，比如内存空间、线程、设备、定时器等。如果把我们的应用比作一个豪宅的主人的话，EAL就是这个豪宅的管家。

在Linux用户空间环境，DPDK APP通过pthread库作为一个用户态程序运行。 设备的PCI信息和地址空间通过 /sys 内核接口及内核模块如uio_pci_generic或igb_uio来发现获取的。 l

EAL通过对hugetlb使用mmap接口来实现物理内存的分配。这部分内存暴露给DPDK服务层，如 Mempool Library。


初始化和运行