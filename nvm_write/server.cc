#include "rlib/lib.hh"
#include "nvm_region.hh"
using namespace rdmaio;
int main(int argc, char **argv)
{
    uint64_t port = 8888;
    int nic_idx = 1;
    uint64_t reg_nic_name = 73;
    uint64_t reg_mem_name = 73;
    std::string host = "127.0.0.1";
    u64 nvm_sz = 2L;                      // Mapped sz (in GB), should be larger than 2MB
    std::string nvm_file = "/dev/dax1.0"; //"Abstracted NVM device"

    RCtrl ctrl(port, host);

    Arc<nvm::MemoryRegion> nvm_region = nullptr;

    // first we open the NIC
    auto nic = RNic::create(RNicInfo::query_dev_names().at(nic_idx)).value();
    // register the nic with name 0 to the ctrl
    RDMA_ASSERT(ctrl.opened_nics.reg(reg_nic_name, nic));

    {
        Arc<RMem> rmem = nullptr;
        u64 sz = static_cast<u64>(nvm_sz) * (1024 * 1024 * 1024L);
        nvm_region = nvm::NVMRegion::create(nvm_file, sz).value();
    
        RDMA_LOG(2) << "init memory region done";

        rmem = std::make_shared<RMem>(
            sz,
            [&nvm_region](u64 s) -> RMem::raw_ptr_t
            {
                return nvm_region->start_ptr();
            },
            [](RMem::raw_ptr_t ptr)
            {
                // currently we donot free the resource, since the region will use
                // to the end
            });

        // allocate a memory (with 1024 bytes) so that remote QP can access it
        RDMA_ASSERT(ctrl.registered_mrs.create_then_reg(
            reg_mem_name, Arc<RMem>(new RMem(1024)),
            ctrl.opened_nics.query(reg_nic_name).value()));
    }
    ctrl.start_daemon();

    RDMA_LOG(2) << "RC nvm server started!";
    while (1)
    {
        // server does nothing because it is RDMA
        sleep(1);
    }
}