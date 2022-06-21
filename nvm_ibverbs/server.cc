#include "rdma_common.hh"
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/mman.h>

void *server_exch_data(RdmaContext *ctx, int sock_port, struct Rdma_connect_info *local_dest, struct Rdma_connect_info *rem_dest)
{
    struct addrinfo *res, *t;
    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM};
    char *service;
    char msg[sizeof "0000:000000:000000:00000000:0000000000000000:00000000000000000000000000000000"];
    int n;
    int sockfd = -1, connfd;
    char gid[33];

    if (asprintf(&service, "%d", sock_port) < 0)
        return NULL;

    n = getaddrinfo(NULL, service, &hints, &res);

    if (n < 0)
    {
        fprintf(stderr, "%s for port %d\n", gai_strerror(n), sock_port);
        free(service);
        return NULL;
    }

    for (t = res; t; t = t->ai_next)
    {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0)
        {
            n = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);
            if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
                break;
            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(res);
    free(service);

    if (sockfd < 0)
    {
        Debug::notifyError("Couldn't listen to port %d", sock_port);
        return NULL;
    }

    listen(sockfd, 1);

    Debug::notifyError("server listen at %d", sock_port);
    connfd = accept(sockfd, NULL, 0);
    close(sockfd);
    if (connfd < 0)
    {
        Debug::notifyError("accept() failed");
    }

    n = read(connfd, msg, sizeof(msg));
    if (n != sizeof(msg))
    {
        Debug::notifyError("%d/%lu: Couldn't read remote address", n, sizeof(msg));
        goto out;
    }
    sscanf(msg, "%x:%x:%x:%x:%lx:%s", &rem_dest->lid, &rem_dest->qpn, &rem_dest->psn, &rem_dest->rKey, &rem_dest->data_vaddr, gid);
    wire_gid_to_gid(gid, &rem_dest->gid);
    gid_to_wire_gid(&local_dest->gid, gid);
    sprintf(msg, "%04x:%06x:%06x:%08x:%16lx:%s", local_dest->lid, local_dest->qpn, local_dest->psn, local_dest->rKey, local_dest->data_vaddr, gid);
    if (write(connfd, msg, sizeof msg) != sizeof msg)
    {
        fprintf(stderr, "Couldn't send local address\n");
        free(rem_dest);
        rem_dest = NULL;
        goto out;
    }

    read(connfd, msg, sizeof msg);

out:
    close(connfd);
    return rem_dest;
}
void opennvm(char **buffer)
{
    Debug::notifyInfo("Strart allocating memory");
    int pmem_file_id = open("/dev/dax1.0",  O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (pmem_file_id < 0)
    {
        Debug::notifyError("open file failed\n");
    }
    size_t mysize =  (size_t)1024 * 1024 * 1024 * 2;
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

        modifyQPtoRTR(qp, remote_info);
        modifyQPtoRTS(qp);

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
    }
    destoryContext(&ctx);
    return 0;
}