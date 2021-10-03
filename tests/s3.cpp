#define BOOST_TEST_MODULE S3
#include <boost/test/included/unit_test.hpp>
#include "../include/comm/Channel.h"
#include "../include/comm/S3.h"
#include <omp.h>

std::map<std::string, std::string> s3_test_params = {
        {"bucket_name", "romanboe-uploadtest"},
        {"s3_region", "eu-central-1"},
        {"timeout", "100"},
        {"max_timeout", "1000"}
};

BOOST_AUTO_TEST_CASE(sending_receiving) {
    auto ch_send = SMI::Comm::Channel::get_channel("S3", s3_test_params);

    // Sending
    int val = 42;
    channel_data buf {reinterpret_cast<char*>(&val), sizeof(val)};
    ch_send->set_peer_id(0);
    ch_send->send(buf, 1);
    ch_send->set_comm_name("Test");

    std::shared_ptr<SMI::Comm::Channel> ch_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
    // Receiving
    int recv;
    channel_data recv_buf {reinterpret_cast<char*>(&recv), sizeof(recv)};
    ch_rcv->set_peer_id(1);
    ch_rcv->recv(recv_buf, 0);
    ch_rcv->set_comm_name("Test");

    ch_send->finalize();
    ch_rcv->finalize();
    BOOST_CHECK_EQUAL(val, recv);
}

BOOST_AUTO_TEST_CASE(sending_receiving_mult_times) {
    auto ch_send = SMI::Comm::Channel::get_channel("S3", s3_test_params);

    // Sending
    int val1 = 42;
    int val2 = 4242;
    ch_send->set_peer_id(0);
    ch_send->set_comm_name("Test");
    ch_send->send({reinterpret_cast<char*>(&val1), sizeof(val1)}, 1);
    ch_send->send({reinterpret_cast<char*>(&val2), sizeof(val2)}, 1);

    std::shared_ptr<SMI::Comm::Channel> ch_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
    // Receiving
    int recv1, recv2;
    ch_rcv->set_peer_id(1);
    ch_rcv->set_comm_name("Test");
    ch_rcv->recv({reinterpret_cast<char*>(&recv1), sizeof(recv1)}, 0);
    ch_rcv->recv({reinterpret_cast<char*>(&recv2), sizeof(recv2)}, 0);

    ch_send->finalize();
    ch_rcv->finalize();

    BOOST_CHECK_EQUAL(val1, recv1);
    BOOST_CHECK_EQUAL(val2, recv2);
}

BOOST_AUTO_TEST_CASE(bcast) {
    constexpr int num_peers = 4;
    std::vector<int> vals(num_peers);
    vals[0] = 42;
    auto ch_send = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    ch_send->set_peer_id(0);
    ch_send->set_comm_name("Test");
    ch_send->bcast({reinterpret_cast<char*>(&vals[0]), sizeof(vals[0])}, 0);

    for (int i = 1; i < num_peers; i++) {
        auto ch_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
        ch_rcv->set_peer_id(i);
        ch_rcv->set_comm_name("Test");
        ch_rcv->bcast({reinterpret_cast<char*>(&vals[i]), sizeof(vals[i])}, 0);
    }
    std::vector<int> expected(num_peers, 42);
    ch_send->finalize();
    BOOST_TEST(vals == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(barrier_unsucc) {
    auto ch_1 = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    ch_1->set_peer_id(0);
    ch_1->set_num_peers(2);
    ch_1->set_comm_name("Test");
    std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
    ch_1->barrier();
    std::chrono::steady_clock::time_point after = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - bef).count();
    BOOST_TEST(elapsed_ms > std::stoi(s3_test_params["max_timeout"]));
}

BOOST_AUTO_TEST_CASE(barrier_succ) {
    auto ch_1 = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    auto ch_2 = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
    ch_1->set_peer_id(0);
    ch_1->set_num_peers(2);
    ch_1->set_comm_name("Test");
    ch_2->set_peer_id(1);
    ch_2->set_num_peers(2);
    ch_2->set_comm_name("Test");
    std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
    #pragma omp parallel num_threads(2)
    {
        int tid = omp_get_thread_num();
        if (tid == 0) {
            ch_1->barrier();
        } else {
            ch_2->barrier();
        }
    }


    std::chrono::steady_clock::time_point after = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - bef).count();

    ch_1->finalize();
    ch_2->finalize();
    BOOST_TEST(elapsed_ms < std::stoi(s3_test_params["max_timeout"]));
}

BOOST_AUTO_TEST_CASE(gather_one) {
    constexpr int num_peers = 2;
    std::vector<int> vals {1,2};
    auto ch_root = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    ch_root->set_peer_id(0);
    ch_root->set_comm_name("Test");
    ch_root->set_num_peers(num_peers);
    std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

    for (int i = 1; i < num_peers; i++) {
        auto ch_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
        ch_rcv->set_peer_id(i);
        ch_rcv->set_num_peers(num_peers);
        ch_rcv->set_comm_name("Test");
        ch_rcv->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()}, {}, 0);
        channels[i - 1] = ch_rcv;
    }
    std::vector<int> vals_rcv(num_peers * 2, 0);
    ch_root->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()},
                    {reinterpret_cast<char*>(vals_rcv.data()), sizeof(vals[0]) * vals_rcv.size()},  0);
    std::vector<int> expected(num_peers * 2, 1);
    for (int i = 1; i < expected.size(); i += 2) {
        expected[i] = 2;
    }
    ch_root->finalize();
    for (int i = 1; i < num_peers; i++) {
        channels[i - 1]->finalize();
    }
    BOOST_TEST(vals_rcv == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(gather_multiple) {
    constexpr int num_peers = 4;
    std::vector<int> vals {1,2};
    auto ch_root = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    ch_root->set_peer_id(0);
    ch_root->set_comm_name("Test");
    ch_root->set_num_peers(num_peers);
    std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

    for (int i = 1; i < num_peers; i++) {
        auto ch_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
        ch_rcv->set_peer_id(i);
        ch_rcv->set_num_peers(num_peers);
        ch_rcv->set_comm_name("Test");
        ch_rcv->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()}, {}, 0);
        channels[i - 1] = ch_rcv;
    }
    std::vector<int> vals_rcv(num_peers * 2, 0);
    ch_root->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()},
                    {reinterpret_cast<char*>(vals_rcv.data()), sizeof(vals[0]) * vals_rcv.size()},  0);
    std::vector<int> expected(num_peers * 2, 1);
    for (int i = 1; i < expected.size(); i += 2) {
        expected[i] = 2;
    }
    ch_root->finalize();
    for (int i = 1; i < num_peers; i++) {
        channels[i - 1]->finalize();
    }
    BOOST_TEST(vals_rcv == expected, boost::test_tools::per_element());
}