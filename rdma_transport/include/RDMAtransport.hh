#pragma once
#ifndef RDMA_TRANSSPORT_HH
#define RDMA_TRANSSPORT_HH

#include <infiniband/verbs.h>
#include <Sherman/Debug.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

namespace myrdma
{

#define PORT_NUM 1
#define MAX_THREAD 1
#define BUF_SIZE 2048
    class RDMAtransport
    {
    public:
        RDMAtransport() {}
        RDMAtransport(uint8_t gidIndex = 1, uint8_t devIndex = -1);
        ~RDMAtransport();
        bool initResource(size_t qp_num);
        bool createQueuePair(ibv_qp_type mode);
        ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize);
        bool modifyQPtoInit(struct ibv_qp *qp);
        bool modifyQPtoRTR(struct ibv_qp *qp, RDMAconnection *remoteinfo);
        bool modifyQPtoRTS(struct ibv_qp *qp, RDMAconnection *localinfo);
        void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]);
        bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
                       uint32_t lkey, uint32_t remoteRKey, int32_t imm, bool isSignaled,
                       uint64_t wrID);
        int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);
        bool client_exch_data(const char *servername, int sock_port, RDMAconnection *local_dest, RDMAconnection *rem_dest);
        void* server_exch_data(int sock_port, RDMAconnection *local_dest, RDMAconnection *rem_dest);

    private:
        // for context
        uint8_t devIndex;
        uint8_t port;
        int gidIndex;
        struct ibv_context *ctx;
        struct ibv_pd *pd;
        uint16_t lid;
        union ibv_gid gid;

        // for connection
        size_t num_qp;
        struct ibv_qp **qp; // 一个qp数组
        struct ibv_cq **cq; // for both sq & rq
        struct ibv_cq *send_cq;
        struct ibv_cq *recv_cq;
        uint32_t qpsMaxDepth;
        uint32_t maxInlineData;

        void fillAhAttr(ibv_ah_attr *attr, RDMAconnection *remoteinfo);
        void wire_gid_to_gid(const char *wgid, union ibv_gid *gid);
        inline void fillSgeWr(ibv_sge &sg, ibv_send_wr &wr, uint64_t source, uint64_t size, uint32_t lkey);
    };

    class RDMAconnection
    {
    public:
        uint32_t get_qpn() { return qpn; }
        uint32_t get_psn() { return psn; }
        uint32_t get_lid() { return lid; }
        union ibv_gid get_gid() { return gid; }
        uint32_t get_rkey() { return rKey; }
        uint64_t get_data_vaddr(){return data_vaddr;}

        void set_qpn(uint32_t Qpn){this->qpn = Qpn;}

    
        uint32_t qpn;      // 这个是每个qp不一样的。
        uint32_t psn;      // Sherman中这个是定值，rdmapingpong中这个是lrand()出来的
        uint32_t lid;      // 这个是ibv_query_port出来的portAttr.lid
        union ibv_gid gid; //这个是ibv_query_gid得到的结果
        uint32_t rKey;
        uint64_t data_vaddr;
        ibv_mr *mr;
    };
}
#endif