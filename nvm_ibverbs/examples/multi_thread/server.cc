#include "rdma_micro.hh"
int main(int argc, char *argv[])
{
    RDMA_microbench server(true);
    if (argc < 2)
    {
        printf("usage: %s threadnum \n eg: %s 2\n", argv[0], argv[0]);
        return 0;
    }
    server.run_server(strtol(argv[1], NULL, 0));
    return 0;
}