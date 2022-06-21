#pragma once

#include <fcntl.h>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <xmmintrin.h>
#include "./memory_region.hh"

namespace nvm
{

  class NVMRegion : public MemoryRegion
  {

    int fd;

    /*!
      create an nvm region in "fsdax" mode
     */
    explicit NVMRegion(const std::string &nvm_file)
        : fd(open(nvm_file.c_str(), O_RDWR))
    {
      if (fd >= 0)
      {
        this->sz =
            (long long int)(std::ifstream(nvm_file, std::ifstream::ate |
                                                        std::ifstream::binary)
                                .tellg());
        this->addr = mmap(nullptr, sz, PROT_WRITE | PROT_READ,
                          MAP_SHARED | MAP_HUGETLB, fd, 0);
        RDMA_ASSERT(false);
      }

      if (!this->valid())
      {
        RDMA_LOG(4) << "not vaild: " << strerror(errno);
      }
    }

    /*!
      create an nvm region in "devdax" mode
      the region in this create can register with RDMA, hopely
     */
    NVMRegion(const std::string &nvm_dev, u64 sz)
        : fd(open(nvm_dev.c_str(), O_RDWR))
    {

      // auto map_flag =
      //       MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB;
      // auto map_flag = MAP_SHARED | MAP_HUGETLB | MAP_POPULATE;
      auto map_flag = MAP_SHARED;
      this->addr =
          mmap(nullptr, sz, PROT_WRITE | PROT_READ, map_flag,
               fd, 0);
      this->sz = sz;
      if (!this->valid())
        RDMA_LOG(4)

            << "mapped fd: " << fd << "; addr: " << addr
            << ", check error:" << strerror(errno);
      // RDMA_ASSERT(false);
    }

    bool valid() override { return fd >= 0 && addr != nullptr; }

  public:
    static Option<Arc<NVMRegion>> create(const std::string &nvm_file)
    {
      auto region = Arc<NVMRegion>(new NVMRegion(nvm_file));
      if (region->valid())
        return region;
      return {};
    }

    static Option<Arc<NVMRegion>> create(const std::string &nvm_file, u64 sz)
    {
      auto region = Arc<NVMRegion>(new NVMRegion(nvm_file, sz));
      if (region->valid())
        return region;
      return {};
    }

    ~NVMRegion()
    {
      if (valid())
      {
        close(fd);
      }
    }

  }; // class NVMRegion

  class HugeRegion : public MemoryRegion
  {
    static u64 align_to_sz(const u64 &x, const usize &align_sz)
    {
      return (((x) + align_sz - 1) / align_sz * align_sz);
    }

  public:
    static ::rdmaio::Option<Arc<HugeRegion>> create(const u64 &sz, const usize &align_sz = (2 << 20))
    {
      auto region = std::make_shared<HugeRegion>(sz, align_sz);
      if (region->valid())
        return region;
      return {};
    }
    explicit HugeRegion(const u64 &sz, const usize &align_sz = (2 << 20))
    {
      this->sz = align_to_sz(sz + align_sz, align_sz);
      char *ptr = (char *)mmap(nullptr,
                              this->sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB, -1, 0);
      if(ptr==MAP_FAILED)
      {
        this->addr = nullptr;
        RDMA_LOG(4) << "error allocating huge page wiht sz: " << this->sz
                  << " aligned with: " << align_sz
                  << "; with error: " << strerror(errno);
      }else
      {
        this->addr = ptr;z
      }
    }
  };

} // namespace nvm
