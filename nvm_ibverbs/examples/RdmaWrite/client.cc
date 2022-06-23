#include "rdma_common.hh"

#define SERVER_IP "192.168.2.206"

int main()
{
    RdmaContext ctx;
    ibv_cq *cq;
    ibv_cq *send_cq;
    ibv_qp *all_qp[MAX_THREAD];
    Rdma_connect_info local_info[MAX_THREAD], remote_info[MAX_THREAD];
    char buf[MAX_THREAD][BUF_SIZE];
    int sock_port = 8888;
    uint8_t uint_gid[16];
    ibv_mr *mr, *localmr;
    int i;

    createContext(&ctx, 1, GID_IDX);
    cq = ibv_create_cq(ctx.ctx, RAW_RECV_CQ_COUNT, NULL, NULL, 0);
    ibv_gid_to_char(&ctx.gid, uint_gid);
    mr = createMemoryRegion((u_int64_t)&buf[0], BUF_SIZE, &ctx);
    localmr = createMemoryRegion((u_int64_t)&buf[1], BUF_SIZE, &ctx);

    for (i = 0; i < MAX_THREAD; i++)
    {
        memset(buf[i], 'b', sizeof(buf[i]));
        createQueuePair(&all_qp[i], IBV_QPT_RC, cq, &ctx);
        auto &qp = all_qp[i];

        local_info[i].qpn = qp->qp_num;
        local_info[i].psn = PSN;
        local_info[i].lid = ctx.lid;
        local_info[i].gid = ctx.gid;
        local_info[i].gidIndex = ctx.gidIndex;
        local_info[i].data_vaddr = reinterpret_cast<uintptr_t>(&buf[i]);
        local_info[i].mr = mr;
        local_info[i].rKey = local_info[i].mr->rkey;

        modifyQPtoInit(qp, &ctx);

        if (!client_exch_data(SERVER_IP, sock_port, &local_info[i], &remote_info[i]))
        {

            Debug::notifyError("exch info failed");
            goto out;
        }

        modifyQPtoRTR(qp, &remote_info[i]);
        modifyQPtoRTS(qp);
        for (int kk = 0; kk < 10; kk++)
        {
            ibv_wc wc;

            memcpy(&buf[i][0] + 10 * kk, "Zhang Rui", sizeof("Zhang Rui"));
            rdmaWrite(qp, (uint64_t)(&buf[i][0] + 10 * kk), remote_info[i].data_vaddr + 10 * kk, sizeof("Zhang Rui"), local_info[i].mr->lkey, remote_info[i].rKey, -1);
            pollWithCQ(cq, 1, &wc);
            printf("write complete 1\n");

            // rdmaRead(qp, (uint64_t)(buf[0]),  remote_info[i].data_vaddr, BUF_SIZE, localmr->lkey,  remote_info[i].rKey);
            // pollWithCQ(cq, 1, &wc);
            // printf("read complete 1\n");
            // for(int b=0;b<BUF_SIZE;b++)
            // {
            //     printf("%c", buf[i][b]);
            // }
            // printf("\n");
            sleep(1);
        }
    }

out:
    destoryContext(&ctx);
    return 0;
}