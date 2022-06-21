#include "nvm_region.hh"
#include "thread.hh"

using namespace rdmaio;

static const int per_socket_cores = 12; // TODO!! hard coded
// const int per_socket_cores = 8;//reserve 2 cores

static int socket_0[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22,
                         24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46};

static int socket_1[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23,
                         25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47};

int BindToCore(int t_id)
{

    if (t_id >= (per_socket_cores * 2))
        return 0;

    int x = t_id;
    int y = 0;

#ifdef SCALE
    assert(false);
    // specific  binding for scale tests
    int mac_per_node = 16 / nthreads; // there are total 16 threads avialable
    int mac_num = current_partition % mac_per_node;

    if (mac_num < mac_per_node / 2)
    {
        y = socket_0[x + mac_num * nthreads];
    }
    else
    {
        y = socket_1[x + (mac_num - mac_per_node / 2) * nthreads];
    }
#else
    // bind ,andway
    if (x >= per_socket_cores)
    {
        // there is no other cores in the first socket
        y = socket_1[x - per_socket_cores];
    }
    else
    {
        y = socket_0[x];
    }

#endif

    // fprintf(stdout,"worker: %d binding %d\n",x,y);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(y, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    return 0;
}

int main(int argc, char **argv)
{
    u64 address_space = 1; // The random read/write space of the registered memory (in GB)
    u64 payload = 256;     // Number of payload to read/write
    uint thread = 8;       // Number of threads to use.
    bool use_read = true;
    int statics;
    u64 use_nic_idx = 1;
    u64 reg_nic_name = 73;
    u64 reg_mem_name = 73;
    u64 coros = 8; //Number of coroutine used per thread.

    using TThread = nvm::Thread<int>;
    std::vector<std::unique_ptr<TThread>> threads;

    auto address_space = static_cast<u64>(address_space) * (1024 * 1024 * 1024L) - payload;

    if (use_read)
    {
        RDMA_LOG(4) << "eval use one-sided READ";
    }
    else
    {
        RDMA_LOG(4) << "eval use one-sided WRITE";
    }
    
    for (uint thread_id = 0; thread_id < thread; ++thread_id)
    {
        threads.push_back(std::make_unique<TThread>([thread_id,
                                                     address_space,
                                                     &statics]() -> int
                                                    {
                                                        auto dram_region = std::make_shared<nvm::DRAMRegion>(2 * 1024 * 1024);
                                                        auto local_mem = std::make_shared<RMem>(
                                                            dram_region->size(),
                                                            [&dram_region](u64 s) -> RMem::raw_ptr_t
                                                            {
                                                                return dram_region->start_ptr();
                                                            },
                                                            [](RMem::raw_ptr_t ptr)
                                                            {
                                                                // currently we donot free the resource,
                                                                // since the region will use to the end
                                                            });
                                                        BindToCore(thread_id);
                                                        // 1. creaate a local QP to use
                                                        // below are platform speific opts
                                                        auto idx = 1;
                                                        auto nic = RNic::create(RNicInfo::query_dev_names().at(idx)).value();
                                                        auto qp = qp::RC::create(nic, qp::QPConfig()).value();

                                                        // 2. create the pair QP at server using CM
                                                        char addr[] = "localhost:8888";
                                                        ConnectManager cm(addr);
                                                        if (cm.wait_ready(1000000, 2) ==
                                                            IOCode::Timeout) // wait 1 second for server to ready, retry 2 times
                                                            RDMA_ASSERT(false) << "cm connect to server timeout";
                                                        char qp_name[64];
                                                        snprintf(qp_name, 64, "%rc:%d:@%d", thread_id, 0);
                                                        u64 key = 0; 
                                                        auto qp_res = cm.cc_rc(qp_name, qp, 1, qp::QPConfig());
                                                        RDMA_ASSERT(qp_res == IOCode::Ok) << std::get<0>(qp_res.desc);

                                                        // 3. create the local MR for usage, and create the remote MR for usage
                                                        auto local_mr = RegHandler::create(local_mem, nic).value();
                                                        auto fetch_res = cm.fetch_remote_mr(1);
                                                        RDMA_ASSERT(fetch_res == IOCode::Ok) << std::get<0>(fetch_res.desc);
                                                        rmem::RegAttr remote_attr = std::get<1>(fetch_res.desc);
                                                        
                                                        /*  fetch dram mr
                                                        fetch_res = cm.fetch_remote_mr(rnic_idx * 73 + 73);
                                                        RDMA_ASSERT(fetch_res == IOCode::Ok) << std::get<0>(fetch_res.desc);
                                                        auto dram_mr = std::get<1>(fetch_res.desc);
                                                        */
                                                       qp->bind_remote_mr(remote_attr);
                                                       qp->bind_local_mr(local_mr->get_reg_attr().value());
                                                       // RDMA connection complete

                                                       u64* test_buf = (u64*)(local_mem->raw_ptr);
                                                       u64 * my_buf = (u64*)((char*)test_buf);

                                                       




                                                       




                                                       

      
      }));
    }
}