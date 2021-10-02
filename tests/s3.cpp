#define BOOST_TEST_MODULE S3
#include <boost/test/included/unit_test.hpp>
#include "../include/comm/Channel.h"

std::map<std::string, std::string> s3_test_params = {
        {"bucket_name", "romanboe-uploadtest"},
        {"s3_region", "eu-central-1"},
        {"timeout", "100"},
        {"max_timeout", "1000"}
};

BOOST_AUTO_TEST_CASE(sending_receiving) {
    auto s3 = SMI::Comm::Channel::get_channel("S3", s3_test_params);

    // Sending
    int val = 42;
    channel_data buf {reinterpret_cast<char*>(&val), sizeof(val)};
    s3->set_peer_id(0);
    s3->send(buf, 1);

    // Receiving
    int recv;
    channel_data recv_buf {reinterpret_cast<char*>(&recv), sizeof(recv)};
    s3->set_peer_id(1);
    s3->recv(recv_buf, 0);

    BOOST_CHECK_EQUAL(val, recv);
}