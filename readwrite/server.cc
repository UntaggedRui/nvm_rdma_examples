#include "rlib/lib.hh"



using namespace rdmaio;
using namespace rdmaio::rmem;


int main(int argc, char **argv){
    int port=1223;
    int nic_idx=1;
    uint64_t reg_nic_name = 73;
    uint64_t reg_mem_name = 73;

    // start a controler, so that others may access it using UDP based channel
    RCtrl ctrl(port);
    RDMA_LOG(4) << "Readwrite server listenes at localhost:" << port;

    // first we open the NIC
    auto nic = RNic::create(RNicInfo::query_dev_names().at(nic_idx)).value();

    // register the nic with name 0 to the ctrl
    RDMA_ASSERT(ctrl.opened_nics.reg(reg_nic_name, nic));

    // allocate a memory (with 1024 bytes) so that remote QP can access it
    RDMA_ASSERT(ctrl.registered_mrs.create_then_reg(
        reg_mem_name, Arc<RMem>(new RMem(1024)), 
        ctrl.opened_nics.query(reg_nic_name).value()));
    
    // initialzie the value so as client can sanity check its content
    u64 *reg_mem = (u64 *)(ctrl.registered_mrs.query(reg_mem_name)
                            .value()
                            ->get_reg_attr()
                            .value()
                            .buf);
    memcpy(reg_mem, "Zhang Rui", strlen("Zhang Rui"));

    // start the listener thread so that client can communicate w it 
    ctrl.start_daemon();

    RDMA_LOG(2) << "Read write server started";

    // run for 20 seconds
    for (uint i = 0;i < 20; ++i) {
        // server does nothing because it is RDMA
        // client will read the reg_mem using RDMA
        RDMA_LOG(4) << "check content: " << (char *)reg_mem;
        sleep(1);
    }
    RDMA_LOG(4) << "server exit!";



}