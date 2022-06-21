#include "rlib/lib.hh"

using namespace rdmaio;
using namespace rdmaio::qp;
void checkDMSupported(struct ibv_context *ctx)
{
    struct ibv_exp_device_attr attrs;
    int kMaxDeviceMemorySize = 0;

    attrs.comp_mask = IBV_EXP_DEVICE_ATTR_UMR;
    attrs.comp_mask |= IBV_EXP_DEVICE_ATTR_MAX_DM_SIZE;

    if (ibv_exp_query_device(ctx, &attrs))
    {
        printf("Couldn't query device attributes\n");
    }

    if (!(attrs.comp_mask & IBV_EXP_DEVICE_ATTR_MAX_DM_SIZE))
    {
        fprintf(stderr, "Can not support Device Memory!\n");
        exit(-1);
    }
    else if (!(attrs.max_dm_size))
    {
    }
    else
    {
        kMaxDeviceMemorySize = attrs.max_dm_size;
        printf("NIC Device Memory is %dKB\n", kMaxDeviceMemorySize / 1024);
    }
}


int main(int argc, char **argv){
    char addr[] = "localhost:1223";
    u64 use_nic_idx = 1;
    u64 reg_nic_name = 73;
    u64 reg_mem_name =73;

    // 1. create a local QP to use
    auto nic = RNic::create(RNicInfo::query_dev_names().at(use_nic_idx)).value();
    auto qp = RC::create(nic, QPConfig()).value();

    ibv_context *ctx = nic->get_ctx();
    checkDMSupported(ctx);








    // 2. create the pair QP at server using CM
    ConnectManager cm(addr);
    if(cm.wait_ready(1000000, 2) ==
        IOCode::Timeout) // wait 1 second for server to ready, retry 2 times
    RDMA_ASSERT(false) << "cm connect to server timeout";

    auto qp_res = cm.cc_rc("client-qp", qp, reg_nic_name, QPConfig());
    RDMA_ASSERT(qp_res == IOCode::Ok) << std::get<0>(qp_res.desc);
    auto key = std::get<1>(qp_res.desc);
    RDMA_LOG(4) << "client fetch QP authentical key: " << key;

    // 3. create the local MR for usage, and create the remote MR for usage
    auto local_mem = Arc<RMem>(new RMem(1024));
    auto local_mr = RegHandler::create(local_mem, nic).value();
    
    auto fetch_res = cm.fetch_remote_mr(reg_mem_name);
    RDMA_ASSERT(fetch_res == IOCode::Ok) << std::get<0>(fetch_res.desc);
    rmem::RegAttr remote_attr = std::get<1>(fetch_res.desc);

    qp->bind_remote_mr(remote_attr);
    qp->bind_local_mr(local_mr->get_reg_attr().value());

    /*This is the example code usage of the fully created RCQP */
    u64 *test_buf = (u64 *)(local_mem->raw_ptr);
    *((char *)test_buf) = 'x';
    char buf[]="woshishabi";
    memcpy(test_buf, buf, sizeof(buf));


    // auto res_s = qp->send_normal(
    //     {.op = IBV_WR_RDMA_WRITE,
    //      .flag = IBV_SEND_SIGNALED,
    //      .len = 1,
    //      .wr_id = 0},
    //     {.local_addr = reinterpret_cast<RMem::raw_ptr_t>(test_buf),
    //     .remote_addr = 1,
    //     .imm_data = 0});
    
    auto res_s = qp->send_normal(
      {.op = IBV_WR_RDMA_WRITE,
       .flags = IBV_SEND_SIGNALED,
       .len = sizeof(buf), // only write one byte
       .wr_id = 0},
      {.local_addr = reinterpret_cast<RMem::raw_ptr_t>(test_buf),
       .remote_addr = 1,
       .imm_data = 0});
    RDMA_ASSERT(res_s == IOCode::Ok);
    auto res_p = qp->wait_one_comp();
    RDMA_ASSERT(res_p == IOCode::Ok);

    RDMA_LOG(4) <<"client write done";

    // finally, some clean up, to delte my created QP at server

    auto del_res = cm.delete_remote_rc("client-qp", key);
    RDMA_ASSERT(del_res == IOCode::Ok) << "delete remote QP error :" <<del_res.desc;

    RDMA_LOG(4) << "client returns";

    return 0; 

}
