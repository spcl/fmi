#include "../../include/utils/ChannelPolicy.h"

#include <utility>

SMI::Utils::ChannelPolicy::ChannelPolicy(std::map<std::string, std::map<std::string, std::string>> channels,
                                         std::map<std::string, std::string> perf_params,
                                         peer_num num_peers) {
    this->channels = std::move(channels);
    this->num_peers = num_peers;
}

std::string SMI::Utils::ChannelPolicy::get_channel(OperationInfo op_info) {
    for (const auto& [channel_name, _] : channels) {
        return channel_name;
    }
}
