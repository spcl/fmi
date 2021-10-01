
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include "../include/comm/S3.h"
#include <iostream>

int main() {
    SMI::Communicator comm(0, 1, "../config/SMI.json");
    SMI::Utils::Configuration config("../config/SMI.json");

    SMI::Comm::Data<int> d = 1;
    //std::cout << d << std::endl;
    SMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    SMI::Comm::Data<void*> d2(data, 5);

    //chann.send(d2, 1, 0);
    std::cout << d.size_in_bytes() << "\n";
    std::cout << d1.size_in_bytes() << "\n";
    std::cout << d2.size_in_bytes() << "\n";

    SMI::Utils::Function<int> f([] (int a, int b) {return a + b;}, true);

    auto backends = config.get_backends();
    for (auto const& [backend_name, backend_params] : backends) {
        std::cout << backend_name << '\n';
    }

    SMI::Comm::S3 s3({{"bucket_name", std::any(std::string("romanboe-uploadtest"))}, {"s3_region", std::any(std::string("eu-central-1"))}});
    comm.register_channel(s3);
    comm.send(d, 0);
    //SMI::Comm::Data<int> ret;
    //s3.download(ret, std::string("test"));
    //std::cout << ret << std::endl;

}