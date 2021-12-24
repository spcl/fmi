#ifndef FMI_CHANNELPOLICY_H
#define FMI_CHANNELPOLICY_H
#include "Common.h"
#include <cstddef>
#include <map>
#include <string>
#include "../comm/Channel.h"

namespace FMI::Utils {
    //! Default channel policy that either selects the fastest or cheapest channel according to the performance model, based on the currently set hint.
    class ChannelPolicy {
    public:
        ChannelPolicy(std::map<std::string, std::shared_ptr<FMI::Comm::Channel>>& channels, peer_num num_peers,
                      double faas_price, Hint hint);

        //! Return the ideal channel for the given operation.
        std::string get_channel(OperationInfo op_info);

        //! Changes the hint that is used by the channel policy.
        void set_hint(Hint hint);

    private:
        std::map<std::string, std::shared_ptr<FMI::Comm::Channel>> &channels;
        peer_num num_peers;
        double faas_price;
        Hint hint;

        double get_faas_price(double execution_time);
    };
}



#endif //FMI_CHANNELPOLICY_H
