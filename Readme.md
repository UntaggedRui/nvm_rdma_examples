# 简介
我用来学习rdma和nvm的一些小代码
# 每个模块的说明

nicmem: 检查网卡上的可用mem的大小

nvm_ibverbs: 使用ibverbs来读写nvm

nvm_write: 使用rlib来读写nvm（未完成）

rdma_transport: 将ibverbs的rdma读写封装成一个（未完成）

readwrite：rlibv2的rdma example

thirdparty：使用到的第三方库，包括rlibv2和从sherman中剥离出来的debug的代码。

# 运行
```
./make.sh
```