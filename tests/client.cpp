
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include "../include/comm/S3.h"
#include "../include/comm/Redis.h"
#include "../include/comm/Direct.h"
#include <iostream>
#include <omp.h>

int main() {
    //SMI::Communicator comm(0, 1, "../config/SMI.json");

    SMI::Comm::Data<int> d = 1;
    //std::cout << d << std::endl;
    SMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    char rcv[5];
    data[0] = '1';
    SMI::Comm::Data<void*> d2(data, 5);

    std::map<std::string, std::string> direct_test_params = {
            {"host", "127.0.0.1"},
            {"port", "10000"}
    };

    #pragma omp parallel num_threads(2)
    {
        int tid = omp_get_thread_num();
        SMI::Comm::Direct comm(direct_test_params);
        comm.set_num_peers(2);
        comm.set_peer_id(tid);
        if (tid == 0)
            comm.send_object({data, sizeof(data)}, 1);
        if (tid == 1)
            comm.recv_object({rcv, sizeof(rcv)}, 0);
    }
    std::cout << rcv[0] << std::endl;



    //comm.send(d, 0);
    //SMI::Comm::Data<int> ret;
    //s3.download(ret, std::string("test"));
    //std::cout << ret << std::endl;

}