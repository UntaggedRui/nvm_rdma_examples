# 简介
使用ibverbs来读写nvm。

如果使用dram将server端的`bool use_nvm = true;`改为false即可。

注意：如果使用NVM, server端需要将nvm用ndctl配置为`devdax`模式，并使用sudo权限启动。

# 每个模块的功能
## src include
`rdma_common.cc`中封装了rdma的建立连接与读写数据的一些基本函数。examples中include rdma_common.hh即可使用这些函数。
## examples
RdmaRead: client端使用rdmaRead读取server端的内存。

RdmaWrite: client端使用rdmaWrite向server端写数据。

send_recv: server端使用send发送数据，client端调用receive接收数据。

multi_thead: 多线程的read