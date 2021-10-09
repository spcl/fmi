#ifndef SMI_CHANNELPOLICY_H
#define SMI_CHANNELPOLICY_H
#include "Common.h"
#include <cstddef>
#include <map>
#include <string>
#include "../comm/Channel.h"

namespace SMI::Utils {
    enum Operation {
        send, bcast, gather, scatter, reduce, allreduce, scan
    };

    struct OperationInfo {
        Operation op;
        std::size_t data_size;
    };

    class ChannelPolicy {
    public:
        ChannelPolicy(std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> &channels, peer_num num_peers);

        std::string get_channel(OperationInfo op_info);

    private:
        std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> &channels;
        peer_num num_peers;
    };
}



#endif //SMI_CHANNELPOLICY_H
