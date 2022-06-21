#include "rdma_common.hh"

#define SERVER_IP "192.168.2.206"
bool client_exch_data(const char *servername, int sock_port, struct Rdma_connect_info *local_dest, struct Rdma_connect_info *rem_dest)
{
    struct addrinfo *res, *t;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM};
    char *service;
    char msg[sizeof "0000:000000:000000:00000000:0000000000000000:00000000000000000000000000000000"];
    int n, sockfd = -1;
    char gid[33];

    if (asprintf(&service, "%d", sock_port) < 0)
        return false;

    n = getaddrinfo(servername, service, &hints, &res);

    if (n < 0)
    {
        Debug::notifyError("%s for %s:%d", gai_strerror(n), servername, sock_port);
        free(service);
        return false;
    }
    for (t = res; t; t = t->ai_next)
    {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0)
        {
            if (!connect(sockfd, t->ai_addr, t->ai_addrlen))
                break;
            close(sockfd);
        }
    }
    freeaddrinfo(res);
    free(service);

    if (sockfd < 0)
    {
        Debug::notifyError("Couldn't connect to %s:%d\n", servername, sock_port);
        return false;
    }

    gid_to_wire_gid(&local_dest->gid, gid);
    sprintf(msg, "%04x:%06x:%06x:%08x:%16lx:%s", local_dest->lid, local_dest->qpn, local_dest->psn, local_dest->rKey, local_dest->data_vaddr, gid);
    ssize_t write_num = write(sockfd, msg, sizeof(msg));
    if (write_num != sizeof(msg))
    {
        Debug::notifyError("send msg %d is Couldn't send local address", write_num);
        goto out;
    }
    if (read(sockfd, msg, sizeof(msg)) != sizeof(msg))
    {
        Debug::notifyError("Couldn't read remote address");
        goto out;
    }
    write(sockfd, "done", sizeof("done"));

    sscanf(msg, "%x:%x:%x:%x:%lx:%s", &rem_dest->lid, &rem_dest->qpn, &rem_dest->psn, &rem_dest->rKey, &rem_dest->data_vaddr, gid);
    wire_gid_to_gid(gid, &rem_dest->gid);

    return true;
out:
    close(sockfd);
    return false;
}
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
        memset(buf[i], 'a', sizeof(buf[i]));
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
            sleep(1);
            memcpy(&buf[i][0] + 10 * kk, "Zhang Rui", sizeof("Zhang Rui"));
            rdmaWrite(qp, (uint64_t)(&buf[i][0] + 10 * kk), remote_info[i].data_vaddr + 10 * kk, sizeof("Zhang Rui"), local_info[i].mr->lkey, remote_info[i].rKey, -1);
            ibv_wc wc;
            pollWithCQ(cq, 1, &wc);
            printf("write complete 1\n");

            rdmaRead(qp, (uint64_t)(&buf[1][0]+10*kk),  remote_info[i].data_vaddr + 10 * kk, 10, localmr->lkey,  remote_info[i].rKey);
            pollWithCQ(cq, 1, &wc);
            printf("read complete 1\n");
        }
    }

out:
    destoryContext(&ctx);
    return 0;
}