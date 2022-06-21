#include "RDMAtransport.hh"
using namespace myrdma;
int main()
{
    char buf[MAX_THREAD][BUF_SIZE];
    int sock_port = 8888;
    RDMAtransport *server = new RDMAtransport(3);



}