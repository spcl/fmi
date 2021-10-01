
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include <iostream>

int main() {
    SMI::Communicator comm(0, 1, "../config/SMI.json");

    SMI::comm::Data<int> d = 1;
    SMI::comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    SMI::comm::Data<void*> d2(data, 5);
    std::cout << d.size_in_bytes() << "\n";
    std::cout << d1.size_in_bytes() << "\n";
    std::cout << d2.size_in_bytes() << "\n";
}