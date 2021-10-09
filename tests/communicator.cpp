#define BOOST_TEST_MODULE SMI
#include <boost/test/unit_test.hpp>

#include <omp.h>
#include "../include/Communicator.h"

BOOST_AUTO_TEST_SUITE(Communicator);

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
    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->bcast(d[peer_id], 0);
    BOOST_CHECK_EQUAL(d[peer_id], 1);
}

BOOST_AUTO_TEST_CASE(scatter) {
    constexpr int num_peers = 4;
    SMI::Comm::Data<std::vector<int>> root_data{{1,2,3,4}};
    std::vector<SMI::Comm::Data<std::vector<int>>> d(num_peers, SMI::Comm::Data<std::vector<int>>(1));
    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->scatter(root_data, d[peer_id], 0);
    BOOST_CHECK_EQUAL(d[peer_id].get()[0], peer_id + 1);
}

BOOST_AUTO_TEST_CASE(gather) {
    constexpr int num_peers = 4;
    SMI::Comm::Data<std::vector<int>> root_data(num_peers);

    std::vector<SMI::Comm::Data<std::vector<int>>> d(num_peers);
    for (int i = 0; i < num_peers; i++) {
        std::vector<int> data(1, i + 1);
        d[i] = SMI::Comm::Data<std::vector<int>>(data);
    }
    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->gather(d[peer_id], root_data, 0);
    std::vector<int> expected{{1,2,3,4}};
    if (peer_id == 0) {
        BOOST_TEST(root_data.get() == expected, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(reduce) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<int>> data(num_peers);
    SMI::Comm::Data<int> res;
    for (int i = 0; i < num_peers; i++) {
        data[i] = i + 1;
    }
    SMI::Utils::Function<int> f([] (auto a, auto b) {return a + b;}, true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->reduce(data[peer_id], res, 0, f);
    if (peer_id == 0) {
        BOOST_CHECK_EQUAL(res, 10);
    }
}

BOOST_AUTO_TEST_CASE(reduce_vector) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<std::vector<int>>> data(num_peers);
    SMI::Comm::Data<std::vector<int>> res(2);
    for (int i = 0; i < num_peers; i++) {
        data[i] = std::vector<int>{i + 1, 2 * (i + 1)};
    }
    SMI::Utils::Function<std::vector<int>> f([] (auto a, auto b) {
                                                    return std::vector<int>{a[0] + b[0], a[1] * b[1]};
                                                },true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->reduce(data[peer_id], res, 0, f);
    if (peer_id == 0) {
        std::vector<int> expected{1 + 2 + 3 + 4, 2 * 4 * 6 * 8};
        BOOST_TEST(res.get() == expected, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(allreduce) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<int>> data(num_peers);
    std::vector<SMI::Comm::Data<int>> res(num_peers);
    for (int i = 0; i < num_peers; i++) {
        data[i] = i + 1;
    }
    SMI::Utils::Function<int> f([] (int a, int b) {return a + b;}, true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->allreduce(data[peer_id], res[peer_id], f);
    BOOST_CHECK_EQUAL(res[peer_id], 10);
}

BOOST_AUTO_TEST_CASE(allreduce_vector) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<std::vector<int>>> data(num_peers);
    std::vector<SMI::Comm::Data<std::vector<int>>> res(num_peers);
    for (int i = 0; i < num_peers; i++) {
        data[i] = std::vector<int>{i + 1, 2 * (i + 1), i + 1};
        res[i] = std::vector<int>(3);
    }
    SMI::Utils::Function<std::vector<int>> f([] (auto a, auto b) {
        return std::vector<int>{a[0] + b[0], a[1] * b[1], std::max(a[2], b[2])};
        },true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->allreduce(data[peer_id], res[peer_id], f);
    std::vector<int> expected{1 + 2 + 3 + 4, 2 * 4 * 6 * 8, 4};
    BOOST_TEST(res[peer_id].get() == expected, boost::test_tools::per_element());

}

BOOST_AUTO_TEST_CASE(scan) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<int>> data(num_peers);
    std::vector<SMI::Comm::Data<int>> res(num_peers);
    for (int i = 0; i < num_peers; i++) {
        data[i] = i + 1;
    }
    std::vector<std::unique_ptr<SMI::Communicator>> comms(num_peers);
    SMI::Utils::Function<int> f([] (int a, int b) {return a + b;}, true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->scan(data[peer_id], res[peer_id], f);
    int prefix_sum = 0;
    for (int i = 0; i <= peer_id; i++) {
        prefix_sum += i + 1;
    }
    BOOST_CHECK_EQUAL(res[peer_id], prefix_sum);
}

BOOST_AUTO_TEST_CASE(scan_vector) {
    constexpr int num_peers = 4;
    std::vector<SMI::Comm::Data<std::vector<int>>> data(num_peers);
    std::vector<SMI::Comm::Data<std::vector<int>>> res(num_peers);
    for (int i = 0; i < num_peers; i++) {
        data[i] = std::vector<int>{i + 1, 2 * (i + 1), i + 1};
        res[i] = std::vector<int>(3);
    }
    SMI::Utils::Function<std::vector<int>> f([] (auto a, auto b) {
        return std::vector<int>{a[0] + b[0], a[1] * b[1], std::max(a[2], b[2])};
        },true, true);

    int peer_id = 0;
    for (int i = 1; i < num_peers; i ++) {
        int pid = fork();
        if (pid == 0) {
            peer_id = i;
            break;
        }
    }
    auto ch = std::make_unique<SMI::Communicator>(peer_id, num_peers, config_path, comm_name);
    ch->scan(data[peer_id], res[peer_id], f);

    int prefix_sum = 0;
    int product = 1;
    for (int i = 0; i <= peer_id; i++) {
        prefix_sum += i + 1;
        product *= 2 * (i + 1);
    }
    BOOST_CHECK_EQUAL(res[peer_id].get()[0], prefix_sum);
    BOOST_CHECK_EQUAL(res[peer_id].get()[1], product);
    BOOST_CHECK_EQUAL(res[peer_id].get()[2], peer_id + 1);

}

BOOST_AUTO_TEST_SUITE_END();