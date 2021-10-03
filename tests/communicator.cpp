#define BOOST_TEST_MODULE SMI
#include <boost/test/unit_test.hpp>

#include <omp.h>
#include "../include/Communicator.h"

BOOST_AUTO_TEST_SUITE(Communicator);


std::map<std::string, std::string> s3_test_params = {
        {"bucket_name", "romanboe-uploadtest"},
        {"s3_region", "eu-central-1"},
        {"timeout", "100"},
        {"max_timeout", "1000"}
};
std::map<std::string, std::map<std::string, std::string>> backends = {
        {"S3", s3_test_params}
};

std::string comm_name = std::to_string(std::time(nullptr));
std::string config_path = "../../config/smi_test.json";

BOOST_AUTO_TEST_CASE(sending_receiving) {
    SMI::Comm::Data<int> d_single = 1;
    SMI::Comm::Data<int> d_single_rcv;
    SMI::Comm::Data<std::vector<int>> d_mult = {{1,2,3}};
    SMI::Comm::Data<std::vector<int>> d_mult_rcv(3);
    std::vector<std::unique_ptr<SMI::Communicator>> comms(2);

    #pragma omp parallel num_threads(2)
    {
        int tid = omp_get_thread_num();
        comms[tid] = std::make_unique<SMI::Communicator>(tid, 2, config_path, comm_name);
        if (tid == 0) {
            comms[tid]->send(d_single, 1);
            comms[tid]->send(d_mult, 1);
        } else {
            comms[tid]->recv(d_single_rcv, 0);
            comms[tid]->recv(d_mult_rcv, 0);
        }
    }
    BOOST_CHECK_EQUAL(d_single, d_single_rcv);
    BOOST_TEST(d_mult.get() == d_mult_rcv.get(), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(bcast) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<int>> d(num_peers);
    d[0] = 1;
    std::vector<std::unique_ptr<SMI::Communicator>> comms(num_peers);

    #pragma omp parallel num_threads(num_peers)
    {
        int tid = omp_get_thread_num();
        comms[tid] = std::make_unique<SMI::Communicator>(tid, num_peers, config_path, comm_name);
        comms[tid]->bcast(d[tid], 0);
    }
    for (int i = 0; i < num_peers; i++) {
        BOOST_CHECK_EQUAL(d[i], 1);
    }
}

BOOST_AUTO_TEST_CASE(scatter) {
    constexpr int num_peers = 4;
    SMI::Comm::Data<std::vector<int>> root_data{{1,2,3,4}};
    std::vector<SMI::Comm::Data<std::vector<int>>> d(num_peers, SMI::Comm::Data<std::vector<int>>(1));
    std::vector<std::unique_ptr<SMI::Communicator>> comms(num_peers);

    #pragma omp parallel num_threads(num_peers)
    {
        int tid = omp_get_thread_num();
        comms[tid] = std::make_unique<SMI::Communicator>(tid, num_peers, config_path, comm_name);
        comms[tid]->scatter(root_data, d[tid], 0);
    }
    for (int i = 0; i < num_peers; i++) {
        BOOST_CHECK_EQUAL(d[i].get()[0], i + 1);
    }
}

BOOST_AUTO_TEST_CASE(gather) {
    constexpr int num_peers = 4;
    SMI::Comm::Data<std::vector<int>> root_data(num_peers);

    std::vector<SMI::Comm::Data<std::vector<int>>> d(num_peers);
    for (int i = 0; i < num_peers; i++) {
        std::vector<int> data(1, i + 1);
        d[i] = SMI::Comm::Data<std::vector<int>>(data);
    }
    std::vector<std::unique_ptr<SMI::Communicator>> comms(num_peers);

    #pragma omp parallel num_threads(num_peers)
    {
        int tid = omp_get_thread_num();
        comms[tid] = std::make_unique<SMI::Communicator>(tid, num_peers, config_path, comm_name);
        comms[tid]->gather(d[tid], root_data, 0);
    }
    std::vector<int> expected{{1,2,3,4}};
    BOOST_TEST(root_data.get() == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END();