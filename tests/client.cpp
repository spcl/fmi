
#include "../include/Communicator.h"
#include "../include/comm/Data.h"
#include "../include/comm/Channel.h"
#include "../include/utils/Function.h"
#include "../include/comm/S3.h"
#include "../include/comm/Redis.h"
#include "../include/comm/Direct.h"
#include <iostream>
#include <omp.h>

std::map<std::string, std::string> s3_test_params = {
        {"bucket_name", "romanboe-uploadtest"},
        {"s3_region", "eu-central-1"},
        {"timeout", "100"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> s3_test_model_params = {
        {"bandwidth", "50.0"},
        {"overhead", "40.4"},
        {"transfer_price", "0.0"},
        {"download_price", "0.00000043"},
        {"upload_price", "0.0000054"}
};

std::map<std::string, std::string> redis_test_params = {
        {"host", "127.0.0.1"},
        {"port", "6379"},
        {"timeout", "1"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> redis_test_model_params = {
        {"bandwidth_single", "100.0"},
        {"bandwidth_multiple", "400.0"},
        {"overhead", "5.2"},
        {"transfer_price", "0.0"},
        {"instance_price", "0.0038"},
        {"requests_per_hour", "1000"},
        {"include_infrastructure_costs", "true"}
};

std::map<std::string, std::string> direct_test_params = {
        {"host", "192.168.0.166"},
        {"port", "10000"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> direct_test_model_params = {
        {"bandwidth", "250.0"},
        {"overhead", "0.34"},
        {"transfer_price", "0.0"},
        {"vm_price", "0.0134"},
        {"requests_per_hour", "1000"},
        {"include_infrastructure_costs", "true"}
};

int main() {
    auto ch_s3 = FMI::Comm::Channel::get_channel("S3", s3_test_params, s3_test_model_params);
    std::cout << ch_s3->get_price(1, 1, 1) << std::endl;
    std::cout << ch_s3->get_latency(1, 1, 1) << std::endl;

    auto ch_redis = FMI::Comm::Channel::get_channel("Redis", redis_test_params, redis_test_model_params);
    std::cout << ch_redis->get_price(1, 1, 1) << std::endl;
    std::cout << ch_redis->get_latency(1, 1, 1) << std::endl;

    auto ch_direct = FMI::Comm::Channel::get_channel("Direct", direct_test_params, direct_test_model_params);
    std::cout << ch_direct->get_price(1, 1, 1) << std::endl;
    std::cout << ch_direct->get_latency(1, 1, 1) << std::endl;

    exit(0);

    FMI::Comm::Data<int> d = 1;
    //std::cout << d << std::endl;
    FMI::Comm::Data<std::vector<int>> d1({1, 2, 3});
    char data[5];
    char rcv[5];
    data[0] = '1';
    FMI::Comm::Data<void*> d2(data, 5);

}