#include "../../include/utils/ChannelPolicy.h"
#include "../../include/comm/Channel.h"

#include <utility>

FMI::Utils::ChannelPolicy::ChannelPolicy(std::map<std::string, std::shared_ptr<FMI::Comm::Channel>>& channels, peer_num num_peers,
                                         double faas_price, Hint hint) : channels(channels), num_peers(num_peers), faas_price(faas_price), hint(hint) {}

std::string FMI::Utils::ChannelPolicy::get_channel(OperationInfo op_info) {
    std::map<std::string, double> times;
    std::map<std::string, double> prices;
    for (const auto& [channel_name, channel] : channels) {
        double latency = channel->get_operation_latency(op_info);
        double channel_price = channel->get_operation_price(op_info);
        double faas_price = get_faas_price(latency);
        double total_price = channel_price + faas_price;
        times[channel_name] = latency;
        prices[channel_name] = total_price;
    }
    if (hint == fast) {
        auto ch = min_element(times.begin(), times.end(),
                              [](const auto& l, const auto& r) { return l.second < r.second; });
        return ch->first;
    } else {
        auto ch = min_element(prices.begin(), prices.end(),
                              [](const auto& l, const auto& r) { return l.second < r.second; });
        return ch->first;
    }
}

double FMI::Utils::ChannelPolicy::get_faas_price(double execution_time) {
    return execution_time * faas_price;
}

void FMI::Utils::ChannelPolicy::set_hint(FMI::Utils::Hint hint) {
    this->hint = hint;
}
