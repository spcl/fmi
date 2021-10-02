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
    auto s3_send = SMI::Comm::Channel::get_channel("S3", s3_test_params);

    // Sending
    int val = 42;
    channel_data buf {reinterpret_cast<char*>(&val), sizeof(val)};
    s3_send->set_peer_id(0);
    s3_send->send(buf, 1);

    std::shared_ptr<SMI::Comm::Channel> s3_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
    // Receiving
    int recv;
    channel_data recv_buf {reinterpret_cast<char*>(&recv), sizeof(recv)};
    s3_rcv->set_peer_id(1);
    s3_rcv->recv(recv_buf, 0);

    s3_send->finalize();
    s3_rcv->finalize();
    BOOST_CHECK_EQUAL(val, recv);
}

BOOST_AUTO_TEST_CASE(sending_receiving_mult_times) {
    auto s3_send = SMI::Comm::Channel::get_channel("S3", s3_test_params);

    // Sending
    int val1 = 42;
    int val2 = 4242;
    s3_send->set_peer_id(0);
    s3_send->send({reinterpret_cast<char*>(&val1), sizeof(val1)}, 1);
    s3_send->send({reinterpret_cast<char*>(&val2), sizeof(val2)}, 1);

    std::shared_ptr<SMI::Comm::Channel> s3_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
    // Receiving
    int recv1, recv2;
    s3_rcv->set_peer_id(1);
    s3_rcv->recv({reinterpret_cast<char*>(&recv1), sizeof(recv1)}, 0);
    s3_rcv->recv({reinterpret_cast<char*>(&recv2), sizeof(recv2)}, 0);

    s3_send->finalize();
    s3_rcv->finalize();

    BOOST_CHECK_EQUAL(val1, recv1);
    BOOST_CHECK_EQUAL(val2, recv2);
}

BOOST_AUTO_TEST_CASE(bcast) {
    constexpr int num_peers = 4;
    std::vector<int> vals(num_peers);
    vals[0] = 42;
    auto s3_sender = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    s3_sender->set_peer_id(0);
    s3_sender->bcast({reinterpret_cast<char*>(&vals[0]), sizeof(vals[0])}, 0);

    for (int i = 1; i < num_peers; i++) {
        auto s3_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
        s3_rcv->set_peer_id(i);
        s3_rcv->bcast({reinterpret_cast<char*>(&vals[i]), sizeof(vals[i])}, 0);
    }
    std::vector<int> expected(num_peers, 42);
    s3_sender->finalize();
    BOOST_TEST(vals == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(barrier_unsucc) {
    auto ch_1 = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    ch_1->set_peer_id(0);
    ch_1->set_num_peers(2);
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
    ch_2->set_peer_id(1);
    ch_2->set_num_peers(2);
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