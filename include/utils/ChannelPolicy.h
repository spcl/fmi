#ifndef SMI_CHANNELPOLICY_H
#define SMI_CHANNELPOLICY_H
#include "Common.h"
#include <cstddef>
#include <map>
#include <string>
#include "../comm/Channel.h"

namespace SMI::Utils {

    class ChannelPolicy {
    public:
        ChannelPolicy(std::map<std::string, std::shared_ptr<SMI::Comm::Channel>>& channels, peer_num num_peers,
                      double faas_price, Hint hint);

        std::string get_channel(OperationInfo op_info);

        void set_hint(Hint hint);

    private:
        std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> &channels;
        peer_num num_peers;
        double faas_price;
        Hint hint;

        double get_faas_price(double execution_time);
    };
}



#endif //SMI_CHANNELPOLICY_H
