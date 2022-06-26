#include "rdma_common.hh"
#include "util.hh"
#define SERVER_IP "192.168.2.206"
class RDMA_microbench
{
public:
    RDMA_microbench(bool use_nvm) : use_nvm(use_nvm)
    {
        buffersize = 1024;
        sock_port = 7777;
        createContext(&ctx, 1, GID_IDX);
        if (use_nvm)
        {
            opennvm(&buf, "/dev/dax1.0", mapsize);
            memset(buf, 'a', buffersize);
        }
        else
        {
            // [MAX_THREAD*BUF_SIZE]
            buf = (char *)malloc(MAX_THREAD * buffersize);
        }
    }
    ~RDMA_microbench()
    {
        destoryContext(&ctx);
        if (use_nvm)
        {
        }
        else
        {
            free(buf);
        }
    }
    void run_server(int thread_num)
    {
        pid = new std::thread *[thread_num];
        for (int i = 0; i < thread_num; i++)
        {
            pid[i] = new std::thread(&RDMA_microbench::run_server_thread, this, i);
        }
        for (int i = 0; i < thread_num; i++)
        {
            pid[i]->join();
        }
        delete[] pid;
    }
    void run_client(int thread_num)
    {
        pid = new std::thread *[thread_num];
        for (int i = 0; i < thread_num; i++)
        {
            pid[i] = new std::thread(&RDMA_microbench::run_client_thread, this, i);
        }
        for (int i = 0; i < thread_num; i++)
        {
            pid[i]->join();
        }
        delete[] pid;
    }

    void run_client_thread(int i)
    {
        char *thread_buf = buf + i * buffersize;
        stick_this_thread_to_core(i);
        cq[i] = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);
        mr[i] = createMemoryRegion((u_int64_t)(thread_buf), buffersize, &ctx);
        memset(thread_buf, 'a' + i, buffersize);
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

        if (!client_exch_data(SERVER_IP, sock_port + i, &local_info[i], &remote_info[i]))
        {

            Debug::notifyError("exch info failed");
        }

        modifyQPtoRTR(qp, &remote_info[i]);
        modifyQPtoRTS(qp);
        for (int kk = 0; kk < 10; kk++)
        {
            ibv_wc wc;
            rdmaRead(qp, (uint64_t)(thread_buf), remote_info[i].data_vaddr, 100, local_info[i].mr->lkey, remote_info[i].rKey);
            pollWithCQ(cq[i], 1, &wc);
            printf("thread %d read complete 1\t", i);
            for (int b = 0; b < buffersize; b++)
            {
                printf("%c", thread_buf[b]);
            }
            printf("\n");
            sleep(1);
        }
    }
    void run_server_thread(int i)
    {
        char *thread_buf = buf + i * buffersize;
        stick_this_thread_to_core(i);
        cq[i] = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);
        mr[i] = createMemoryRegion((u_int64_t)(thread_buf), buffersize, &ctx);
        memset(thread_buf, 'A' + i, buffersize);
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
#if WRITE
        while (1)
        {
            for (int kk = 0; kk < buffersize; kk++)
            {
                if (use_nvm)
                {
                    printf("%c", nvmbuf[kk]);
                }
                else
                {
                    printf("%c", buf[0][kk]);
                }
            }
            printf("\n");
            sleep(1);
        }
#else
        int kk = 0;
        while (1)
        {
            sprintf(thread_buf + kk * 17, "thread %d round %d", i, kk);
            sleep(1);
            kk++;
        }

#endif
    }
    RdmaContext ctx;
    ibv_cq *cq[MAX_THREAD];
    ibv_mr *mr[MAX_THREAD];
    ibv_qp *all_qp[MAX_THREAD];
    Rdma_connect_info local_info[MAX_THREAD], remote_info[MAX_THREAD];
    char *buf = NULL;
    uint32_t buffersize;
    int sock_port;
    bool use_nvm = true;
    size_t mapsize = (size_t)1024 * 1024 * 1024 * 2;
    std::thread **pid;
};