#include "../../include/comm/Channel.h"
#include "../../include/comm/S3.h"
#include "../../include/comm/Redis.h"
#include "../../include/comm/Direct.h"

std::shared_ptr<SMI::Comm::Channel> SMI::Comm::Channel::get_channel(std::string name, std::map<std::string, std::string> params,
                                                                    std::map<std::string, std::string> perf_params) {
    if (name == "S3") {
        return std::make_shared<S3>(params, perf_params);
    } else if (name == "Redis") {
        return std::make_shared<Redis>(params, perf_params);
    } else if (name == "Direct") {
        return std::make_shared<Direct>(params, perf_params);
    } else {
        throw "Unknown channel name passed";
    }
}

void SMI::Comm::Channel::gather(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) {
    if (peer_id != root) {
        send(sendbuf, root);
    } else {
        auto buffer_length = sendbuf.len;
        for (int i = 0; i < num_peers; i++) {
            if (i == root) {
                std::memcpy(recvbuf.buf + root * buffer_length, sendbuf.buf, buffer_length);
            } else {
                channel_data peer_data {recvbuf.buf + i * buffer_length, buffer_length};
                recv(peer_data, i);
            }
        }
    }
}

void SMI::Comm::Channel::scatter(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) {
    if (peer_id == root) {
        auto buffer_length = recvbuf.len;
        for (int i = 0; i < num_peers; i++) {
            if (i == root) {
                std::memcpy(recvbuf.buf, sendbuf.buf + root * buffer_length, buffer_length);
            } else {
                channel_data peer_data {sendbuf.buf + i * buffer_length, buffer_length};
                send(peer_data, i);
            }
        }
    } else {
        recv(recvbuf, root);
    }
}

void SMI::Comm::Channel::allreduce(channel_data sendbuf, channel_data recvbuf, raw_function f) {
    reduce(sendbuf, recvbuf, 0, f);
    bcast(recvbuf, 0);
}
