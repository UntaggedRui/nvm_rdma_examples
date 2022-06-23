#include "rdma_common.hh"
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/mman.h>
#define WRITE 0

void opennvm(char **buffer)
{
    Debug::notifyInfo("Start allocating memory");
    int pmem_file_id = open("/dev/dax1.0", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (pmem_file_id < 0)
    {
        Debug::notifyError("open file failed\n");
    }
    size_t mysize = (size_t)1024 * 1024 * 1024 * 2;
    *buffer = (char *)mmap(0, mysize, PROT_READ | PROT_WRITE, 0x80003, pmem_file_id, 0);
    printf("%p\n", *buffer);
}
int main()
{
    RdmaContext ctx;
    ibv_cq *cq;
    ibv_cq *send_cq;
    ibv_qp *all_qp[MAX_THREAD];
    Rdma_connect_info local_info[MAX_THREAD], remote_info[MAX_THREAD];
    char buf[MAX_THREAD][BUF_SIZE];
    char *nvmbuf = NULL;
    int sock_port = 8888;
    uint8_t uint_gid[16];
    bool use_nvm = true;

    createContext(&ctx, 1, GID_IDX);
    ibv_gid_to_char(&ctx.gid, uint_gid);

    cq = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);

    for (int i = 0; i < MAX_THREAD; i++)
    {
        if (use_nvm)
        {
            opennvm(&nvmbuf);
            memset(nvmbuf, 'a', BUF_SIZE);
        }
        else
        {
            memset(buf[i], 'a', sizeof(buf[i]));
        }
        createQueuePair(&all_qp[i], IBV_QPT_RC, cq, &ctx);
        auto &qp = all_qp[i];

        local_info[i].qpn = qp->qp_num;
        local_info[i].psn = PSN;
        local_info[i].lid = ctx.lid;
        local_info[i].gid = ctx.gid;
        local_info[i].gidIndex = ctx.gidIndex;
        if (use_nvm)
        {
            local_info[i].mr = createMemoryRegion((u_int64_t)nvmbuf, BUF_SIZE, &ctx);
            local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(local_info[i].mr->addr);
        }
        else
        {
            local_info[i].mr = createMemoryRegion((u_int64_t)&buf[i], BUF_SIZE, &ctx);
            local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(&buf[i]);
        }
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
                    memcpy(nvmbuf+kk*10, "Zhang RUI", 10);
                }
                else
                {
                    printf("%c", buf[0][kk]);
                }
                sleep(1);

            }
            // printf("\n");
        }

#endif
    }
    destoryContext(&ctx);
    return 0;
}