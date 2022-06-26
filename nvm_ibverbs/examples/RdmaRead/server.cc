#include "rdma_common.hh"
#include "util.hh"
#define WRITE 0

int main()
{
    RdmaContext ctx;
    ibv_cq *cq;
    ibv_qp *all_qp[MAX_THREAD];
    Rdma_connect_info local_info[MAX_THREAD], remote_info[MAX_THREAD];
    char *buf = NULL;
    int buffersize = 1024;
    int sock_port = 8888;
    bool use_nvm = true;
    size_t mysize = (size_t)1024 * 1024 * 1024 * 2;
    std::thread **pid = new std::thread *[MAX_THREAD];

    createContext(&ctx, 1, GID_IDX);

    if (use_nvm)
    {
        opennvm(&buf, "/dev/dax1.0", mysize);
        memset(buf, 'a', BUF_SIZE);
    }
    else
    {
        // [MAX_THREAD*BUF_SIZE]
        buf = (char *)malloc(MAX_THREAD * BUF_SIZE);
    }

    cq = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);

    for (int i = 0; i < MAX_THREAD; i++)
    {
        createQueuePair(&all_qp[i], IBV_QPT_RC, cq, &ctx);
        auto &qp = all_qp[i];

        local_info[i].qpn = qp->qp_num;
        local_info[i].psn = PSN;
        local_info[i].lid = ctx.lid;
        local_info[i].gid = ctx.gid;
        local_info[i].gidIndex = ctx.gidIndex;

        local_info[i].mr = createMemoryRegion((u_int64_t)buf, BUF_SIZE, &ctx);
        local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(local_info[i].mr->addr);

        local_info[i].rKey = local_info[i].mr->rkey;

        modifyQPtoInit(qp, &ctx);

        local_info[i].lid = ctx.lid;
        server_exch_data(&ctx, sock_port, &local_info[i], &remote_info[i]);

        modifyQPtoRTR(qp, &remote_info[i]);
        modifyQPtoRTS(qp);
#if WRITE
        while (1)
        {
            for (int kk = 0; kk < BUF_SIZE; kk++)
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
        while (1)
        {
            for (int kk = 0; kk < BUF_SIZE; kk++)
            {
                if (use_nvm)
                {
                    memcpy(buf + kk * 10, "Zhang RUI", 10);
                }
                else
                {
                    printf("%c", buf[kk]);
                }
                sleep(1);
            }
            // printf("\n");
        }

#endif
    }
    if (use_nvm)
    {
    }
    else
    {
        free(buf);
    }
    destoryContext(&ctx);
    return 0;
}