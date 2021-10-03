
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include "../include/comm/S3.h"
#include <iostream>

int main() {
    //SMI::Communicator comm(0, 1, "../config/SMI.json");
    SMI::Utils::Configuration config("../config/SMI.json");

    SMI::Comm::Data<int> d = 1;
    //std::cout << d << std::endl;
    SMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    SMI::Comm::Data<void*> d2(data, 5);

    SMI::Utils::Function<int> f([] (int a, int b) {return a + b;}, true, true);



    //comm.send(d, 0);
    //SMI::Comm::Data<int> ret;
    //s3.download(ret, std::string("test"));
    //std::cout << ret << std::endl;

}