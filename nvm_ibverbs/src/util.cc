#include "util.hh"
int stick_this_thread_to_core(int core_id)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || core_id >= num_cores)
        return EINVAL;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
void opennvm(char **buffer, const std::string pm_file_path, size_t mysize)
{
    if (geteuid() != 0)
    {
        Debug::notifyError("running nvm with daxmod need sudo");
        exit(-1);
    }
    Debug::notifyInfo("Start allocating memory");
    int pmem_file_id = open(pm_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (pmem_file_id < 0)
    {
        Debug::notifyError("open file failed\n");
    }
    
    *buffer = (char *)mmap(0, mysize, PROT_READ | PROT_WRITE, 0x80003, pmem_file_id, 0);
    printf("%p\n", *buffer);
}