#pragma once
#ifndef RDMA_COMMON_HH
#define RDMA_COMMON_HH

#include <infiniband/verbs.h>
#include <Sherman/Debug.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>

#define RAW_RECV_CQ_COUNT 128
#define MAX_THREAD 10
#define BUF_SIZE 4096
#define PSN 3185
#define GID_IDX 3
#define PORT_NUM 1

struct RdmaContext
{
  uint8_t devIndex;
  uint8_t port;
  int gidIndex;

  ibv_context *ctx;
  ibv_pd *pd;

  uint16_t lid;
  union ibv_gid gid;

  RdmaContext() : ctx(NULL), pd(NULL) {}
};

struct Rdma_connect_info
{
  uint32_t qpn;      // 这个是每个qp不一样的。
  uint32_t psn;      // Sherman中这个是定值，rdmapingpong中这个是lrand()出来的
  uint32_t lid;      // 这个是ibv_query_port出来的portAttr.lid
  union ibv_gid gid; //这个是ibv_query_gid得到的结果
  uint8_t gidIndex;

  // uint8_t gid[16];

  uint32_t rKey;
  uint64_t data_vaddr;
  ibv_mr *mr;
};

void *server_exch_data(RdmaContext *ctx, int sock_port, struct Rdma_connect_info *local_dest, struct Rdma_connect_info *rem_dest);
bool client_exch_data(const char *servername, int sock_port, struct Rdma_connect_info *local_dest, struct Rdma_connect_info *rem_dest);
bool createContext(RdmaContext *context, uint8_t port = 1, int gidIndex = 1,
                   uint8_t devIndex = 0);

bool destoryContext(RdmaContext *context);

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *cq,
                     RdmaContext *context, uint32_t qpsMaxDepth = 128,
                     uint32_t maxInlineData = 0);
bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *send_cq,
                     ibv_cq *recv_cq, RdmaContext *context,
                     uint32_t qpsMaxDepth = 128, uint32_t maxInlineData = 0);
ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize, RdmaContext *ctx);

bool modifyQPtoInit(struct ibv_qp *qp, RdmaContext *context);
bool modifyQPtoRTR(struct ibv_qp *qp, struct Rdma_connect_info *remoteinfo);
bool modifyQPtoRTS(struct ibv_qp *qp);
bool modifyUDtoRTS(struct ibv_qp *qp, RdmaContext *context);
void fillAhAttr(ibv_ah_attr *attr, uint32_t remoteLid, uint8_t *remoteGid,
                RdmaContext *context);

bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
               uint32_t lkey, uint32_t remoteRKey, int32_t imm = -1,
               bool isSignaled = true, uint64_t wrID = 0);
bool rdmaRead(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
              uint32_t lkey, uint32_t remoteRKey, bool signal = true,
              uint64_t wrID = 0);
bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              int32_t imm = -1);
bool rdmaReceive(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
                 uint64_t wr_id = 0);
int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);

void wire_gid_to_gid(const char *wgid, union ibv_gid *gid);
void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]);
void ibv_gid_to_char(union ibv_gid *ibvgid, uint8_t *gid);






#endif