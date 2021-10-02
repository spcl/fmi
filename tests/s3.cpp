#define BOOST_TEST_MODULE S3
#include <boost/test/included/unit_test.hpp>
#include "../include/comm/Channel.h"
#include "../include/comm/S3.h"

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

    BOOST_CHECK_EQUAL(val, recv);
}

BOOST_AUTO_TEST_CASE(bcast) {
    constexpr int num_peers = 4;
    std::vector<int> vals(num_peers);
    vals[0] = 42;
    auto s3_sender = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    s3_sender->set_peer_id(0);
    s3_sender->bcast({reinterpret_cast<char*>(&vals[0]), sizeof(vals[0])}, 0);

    for (int i = 1; i < num_peers; i++) {
        std::shared_ptr<SMI::Comm::Channel> s3_rcv = std::make_shared<SMI::Comm::S3>(s3_test_params, false);
        s3_rcv->set_peer_id(i);
        s3_rcv->bcast({reinterpret_cast<char*>(&vals[i]), sizeof(vals[i])}, 0);
    }
    std::vector<int> expected(num_peers, 42);
    BOOST_TEST(vals == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(barrier) {
    //TODO
    auto s3_1 = SMI::Comm::Channel::get_channel("S3", s3_test_params);
    s3_1->set_peer_id(0);
    s3_1->set_num_peers(2);

    s3_1->barrier();
}