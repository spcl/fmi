
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include <iostream>

int main() {
    SMI::Communicator comm(0, 1, "../config/SMI.json");

    SMI::Comm::Data<int> d = 1;
    SMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    SMI::Comm::Data<void*> d2(data, 5);

    SMI::Comm::Channel chann;
    //chann.send(d2, 1, 0);
    std::cout << d.size_in_bytes() << "\n";
    std::cout << d1.size_in_bytes() << "\n";
    std::cout << d2.size_in_bytes() << "\n";

    SMI::Utils::Function<int> f([] (int a, int b) {return a + b;}, true);
}