#include "../../include/utils/ChannelPolicy.h"
#include "../../include/comm/Channel.h"

#include <utility>

SMI::Utils::ChannelPolicy::ChannelPolicy(std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> &channels,
                                         peer_num num_peers) : channels(channels), num_peers(num_peers) {}

std::string SMI::Utils::ChannelPolicy::get_channel(OperationInfo op_info) {
    for (const auto& [channel_name, _] : channels) {
        return channel_name;
    }
}
