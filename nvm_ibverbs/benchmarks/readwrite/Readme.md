# 使用方法
1. 一些参数
```bash
    -benchmarks (The type of benchmarks) type: string default: "read"
    -block_size (The size of each IO) type: int64 default: 128
    -buf_size (The size of pmem device) type: uint64 default: 1073741824
    -is_server (True means Server(bypass), False means Client (Read or Write))
      type: bool default: false
    -ops (Total number of operations read or write) type: int64 default: 100
    -pmem_path (The path of the pmem device) type: string
      default: "/dev/dax1.0"
    -random (Perform random IO or not) type: bool default: true
    -server_ip (The ip of the server) type: string default: "192.168.100.2"
    -sock_port (The port of the server) type: int32 default: 45678
    -thread_num (run thread) type: int32 default: 1
    -use_pmem (Use PMEM or DRAM) type: bool default: false
```
## 使用dram作为服务端的RDMA内存
1. 编译
  ```bash
  cmake  ..
  make -j$(nproc)
  ```

2. 服务端运行
   ```bash
   ./rdma_rw_bench -is_server
   ```

3. 客户端运行
    ```bash
   ./nvm_ibverbs/benchmarks/rdma_rw_bench  -server_ip 10.176.22.221 -ops 100000
   ```

4. 客户端显示结果
  ```bash

  ```

## 使用NVM作为服务端的RDMA内存
1. 编译
  ```bash
  cmake -DUSE_NVM=ON ..
  make -j$(nproc)
  ```
  
2. 服务端运行
  ```bash
   sudo ./nvm_ibverbs/benchmarks/rdma_rw_bench -is_server -use_pmem true
   ```

3. 客户端运行
    ```bash
   ./nvm_ibverbs/benchmarks/rdma_rw_bench  -server_ip 10.176.22.221 -ops 100000
   ```

4. 客户端显示结果
  ```bash
  RDMA seq read lat: Count: 100000  Average: 4.3556  StdDev: 3.06
Min: 0.0000  Median: 4.3322  Max: 960.6490
------------------------------------------------------
[       3,       4 )   31122  31.122%  31.122% ######
[       4,       5 )   56825  56.825%  87.947% ###########
[       5,       6 )   11818  11.818%  99.765% ##
[       6,       7 )     208   0.208%  99.973%
[       7,       8 )      10   0.010%  99.983%
[       8,       9 )       7   0.007%  99.990%
[       9,      10 )       1   0.001%  99.991%
[      10,      12 )       1   0.001%  99.992%
[      12,      14 )       2   0.002%  99.994%
[      16,      18 )       2   0.002%  99.996%
[      20,      25 )       2   0.002%  99.998%
[      45,      50 )       1   0.001%  99.999%
```

