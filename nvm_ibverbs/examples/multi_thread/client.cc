#include "rdma_micro.hh"
int main(int argc,  char *argv[])
{
    RDMA_microbench client(false);
    if (argc < 2)
    {
        printf("usage: %s threadnum \n eg: %s 2\n", argv[0], argv[0]);
        return 0;
    }
    client.run_client(strtol(argv[1], NULL, 0));
    return 0;
}