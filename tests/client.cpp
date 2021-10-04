
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include "../include/comm/S3.h"
#include "../include/comm/Redis.h"
#include <iostream>

int main() {
    //SMI::Communicator comm(0, 1, "../config/SMI.json");

    SMI::Comm::Data<int> d = 1;
    //std::cout << d << std::endl;
    SMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    SMI::Comm::Data<void*> d2(data, 5);

    std::map<std::string, std::string> redis_test_params = {
            {"host", "127.0.0.1"},
            {"port", "6379"},
            {"timeout", "1"},
            {"max_timeout", "1000"}
    };

    SMI::Comm::Redis comm(redis_test_params);
    data[0] = '1';
    char rcv[5];
    comm.upload_object({data, sizeof(data)}, "Test2");
    comm.download_object({rcv, sizeof(rcv)}, "Test2");
    comm.delete_object("Test");
    auto keys = comm.get_object_names();
    std::cout << rcv[0] << std::endl;
    for (auto key : keys)
        std::cout << key << '\n';



    //comm.send(d, 0);
    //SMI::Comm::Data<int> ret;
    //s3.download(ret, std::string("test"));
    //std::cout << ret << std::endl;

}