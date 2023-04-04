#include "rdma_bench.hh"

// common
DEFINE_bool(is_server, false, "True means Server(bypass), False means Client (Read or Write)");
DEFINE_int32(thread_num, 1, "run thread");

// RDMA

// Server / Host
DEFINE_bool(use_pmem, false, "Use PMEM or DRAM");
DEFINE_string(pmem_path, "/dev/dax1.0", "The path of the pmem device");
DEFINE_uint64(buf_size, 1UL * 1024 * 1024 * 1024, "The size of pmem device");
DEFINE_string(server_ip, "192.168.100.2", "The ip of the server");
DEFINE_int32(sock_port, 45678, "The port of the server");

// Client
DEFINE_int64(block_size, 128, "The size of each IO");
DEFINE_int64(ops, 100, "Total number of operations read or write");
DEFINE_bool(random, true, "Perform random IO or not");
DEFINE_string(benchmarks, "read", "The type of benchmarks");

RDMA_bench::RDMA_bench()
{
    int is_pmem;
    use_nvm = FLAGS_use_pmem;
    sock_port = FLAGS_sock_port;
    buf_size = FLAGS_buf_size;
    block_size = FLAGS_block_size;
    createContext(&ctx, 1, GID_IDX);
    if (FLAGS_is_server)
    {
        if (use_nvm)
        {
#ifdef USE_NVM
            buf = (char *)pmem_map_file(FLAGS_pmem_path.c_str(), 0, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
            if (buf == nullptr)
            {
                fprintf(stderr, "map pmem file failed : %s\n", strerror(errno));
            }
#else
            printf("please recmake with cmake -DUSE_NVM=ON");
#endif
        }
        else
        {
            buf = new char[buf_size];
        }
    }
    else
    {
        buf = new char[buf_size];
    }
}
RDMA_bench::~RDMA_bench()
{
    // destoryContext(&ctx);
    /**
     * 调用ibv_destroy_qp销毁QP（Queue Pair）对象，以取消所有未完成的操作。
        调用ibv_destroy_cq销毁CQ（Completion Queue）对象，以确保不会再收到任何完成事件。
        调用ibv_dereg_mr取消所有已注册的内存区域，以确保不再引用这些区域。
        调用ibv_dealloc_pd销毁PD（Protection Domain）对象，以确保取消任何与该PD关联的资源。
        最后，关闭Socket连接并释放相关的资源（例如，文件描述符）。
    */
    // 销毁qp
    int i;
    for (i = 0; i < this->thread_num; i++)
    {
        ibv_destroy_qp(this->all_qp[i]);
        ibv_destroy_cq(this->cq[i]);
        ibv_dereg_mr(this->mr[i]);
    }
    if (ctx.pd)
    {
        int ans = ibv_dealloc_pd(ctx.pd);
        if (ans)
        {
            Debug::notifyError("Failed to deallocate PD, errorcode is - %s", strerror(errno));
        }
    }

    if (ctx.ctx)
    {
        if (ibv_close_device(ctx.ctx))
        {
            Debug::notifyError("failed to close device context");
        }
    }

    if (FLAGS_is_server)
    {
        if (use_nvm)
        {
#ifdef USE_NVM
            pmem_unmap(buf, mapped_len);
#endif
        }
        else
        {
            delete[] buf;
        }
    }
    else
    {
        delete[] buf;
    }
}
void RDMA_bench::run_server(int thread_num)
{
    this->thread_num = thread_num;
    pid = new std::thread *[this->thread_num];
    for (int i = 0; i < this->thread_num; i++)
    {
        pid[i] = new std::thread(&RDMA_bench::run_server_thread, this, i);
        // rc = pthread_create(&threads[i], NULL, PThreadSearch, (void *)&td[i]);
    }
    for (int i = 0; i < this->thread_num; i++)
    {
        pid[i]->join();
    }
    delete[] pid;
}
void RDMA_bench::run_client(int thread_num)
{
    this->thread_num = thread_num;
    pid = new std::thread *[this->thread_num];
    for (int i = 0; i < this->thread_num; i++)
    {
        pid[i] = new std::thread(&RDMA_bench::run_client_thread, this, i);
    }
    for (int i = 0; i < this->thread_num; i++)
    {
        pid[i]->join();
    }
    delete[] pid;
}
void RDMA_bench::run_client_thread(int i)
{
    char *thread_buf = buf + i * block_size;
    stick_this_thread_to_core(i);
    cq[i] = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);
    mr[i] = createMemoryRegion((u_int64_t)(thread_buf), block_size, &ctx);
    memset(thread_buf, 'a' + i, block_size);
    createQueuePair(&all_qp[i], IBV_QPT_RC, cq[i], &ctx);
    auto &qp = all_qp[i];

    local_info[i].qpn = qp->qp_num;
    local_info[i].psn = PSN;
    local_info[i].lid = ctx.lid;
    local_info[i].gid = ctx.gid;
    local_info[i].gidIndex = ctx.gidIndex;
    local_info[i].mr = mr[i];
    local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(local_info[i].mr->addr);
    local_info[i].rKey = local_info[i].mr->rkey;

    modifyQPtoInit(qp, &ctx);
    Debug::debugItem("begin connect to server");
    if (!client_exch_data(FLAGS_server_ip.c_str(), sock_port + i, &local_info[i], &remote_info[i]))
    {

        Debug::notifyError("exch info failed");
    }
    Debug::debugItem("begin modifyQPtoRTR");
    modifyQPtoRTR(qp, &remote_info[i]);
    Debug::debugItem("begin modifyQPtoRTS");
    modifyQPtoRTS(qp);
    Debug::debugItem("rdma coneect complete");

    ibv_wc wc;
    if (FLAGS_benchmarks == "read")
    {
        auto hist = leveldb::Histogram();
        for (int kk = 0; kk < FLAGS_ops; kk++)
        {
            auto start = std::chrono::high_resolution_clock::now();
            rdmaRead(qp, (uint64_t)(thread_buf), remote_info[i].data_vaddr, block_size, local_info[i].mr->lkey, remote_info[i].rKey);
            pollWithCQ(cq[i], 1, &wc);
            auto end1 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> dma_read_lat = end1 - start;
            hist.Add(dma_read_lat.count());
#if 0            
                printf("thread %d read complete 1\t", i);
                for (int b = 0; b < block_size; b++)
                {
                    printf("%c", thread_buf[b]);
                }
                printf("\n");
                sleep(1);
#endif
        }
        printf("RDMA seq read lat: %s\n", hist.ToString().c_str());
    }
    else if (FLAGS_benchmarks == "write")
    {
    }
    else
    {
        printf("error benchmark type\n");
    }
}

void RDMA_bench::run_server_thread(int i)
{
    char *thread_buf = buf + i * block_size;
    stick_this_thread_to_core(i);
    cq[i] = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);
    mr[i] = createMemoryRegion((u_int64_t)(thread_buf), block_size, &ctx);
    memset(thread_buf, 'A' + i, block_size);
    createQueuePair(&all_qp[i], IBV_QPT_RC, cq[i], &ctx);
    auto &qp = all_qp[i];

    local_info[i].qpn = qp->qp_num;
    local_info[i].psn = PSN;
    local_info[i].lid = ctx.lid;
    local_info[i].gid = ctx.gid;
    local_info[i].gidIndex = ctx.gidIndex;
    local_info[i].mr = mr[i];
    local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(local_info[i].mr->addr);
    local_info[i].rKey = local_info[i].mr->rkey;

    modifyQPtoInit(qp, &ctx);

    server_exch_data(&ctx, sock_port + i, &local_info[i], &remote_info[i]);

    modifyQPtoRTR(qp, &remote_info[i]);
    modifyQPtoRTS(qp);
    if (FLAGS_benchmarks == "read")
    {
        int kk = 0;
        while (1)
        {
            sprintf(thread_buf + kk * 17, "thread %d round %d", i, kk);
            sleep(1);
            kk++;
        }
    }
    else if (FLAGS_benchmarks == "write")
    {
        while (1)
        {
            for (int kk = 0; kk < block_size; kk++)
            {
                if (use_nvm)
                {
                    printf("%c", buf[kk]);
                }
                else
                {
                    printf("%c", buf[kk]);
                }
            }
            printf("\n");
            sleep(1);
        }
    }
    else
    {
        printf("error benchmark type\n");
    }
}

int main(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    RDMA_bench mybench;
    if (FLAGS_is_server)
    {
        mybench.run_server(FLAGS_thread_num);
    }
    else
    {
        mybench.run_client(FLAGS_thread_num);
    }
}