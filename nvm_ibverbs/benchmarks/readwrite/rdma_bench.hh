#include "rdma_common.hh"
#include "util.hh"
#ifdef USE_NVM
#include <libpmem.h>
#endif
#include <gflags/gflags.h>
#include <chrono>
#include "histogram.h"
class RDMA_bench
{
public:
    RDMA_bench();
    ~RDMA_bench();
    void run_server(int thread_num);
    void run_client(int thread_num);
    void run_client_thread(int i);
    void run_server_thread(int i);
private:
    RdmaContext ctx;
    ibv_cq *cq[MAX_THREAD];
    ibv_mr *mr[MAX_THREAD];
    ibv_qp *all_qp[MAX_THREAD];
    Rdma_connect_info local_info[MAX_THREAD], remote_info[MAX_THREAD];
    char *buf = NULL;
    size_t buf_size;
    uint64_t block_size;
    int sock_port;
    bool use_nvm = true;
    bool is_server;
    size_t mapsize = (size_t)1024 * 1024 * 1024 * 2;
    std::thread **pid;
    size_t mapped_len;
};