#include "../../include/comm/Channel.h"
#include "../../include/comm/S3.h"

std::shared_ptr<SMI::Comm::Channel> SMI::Comm::Channel::get_channel(std::string name, std::map<std::string, std::string> params) {
    if (name == "S3") {
        return std::make_shared<S3>(params);
    } else {
        throw "Unknown channel name passed";
    }
}

void SMI::Comm::Channel::gather(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) {
    if (peer_id != root) {
        send(sendbuf, root);
    } else {
        auto buffer_length = sendbuf.len;
        std::memcpy(recvbuf.buf + root * buffer_length, sendbuf.buf, buffer_length);
        for (int i = 0; i < num_peers; i++) {
            if (i == root) {
                continue;
            }
            channel_data peer_data {recvbuf.buf + i * buffer_length, buffer_length};
            recv(peer_data, i);
        }
    }
}
