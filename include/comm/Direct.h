#ifndef SMI_DIRECT_H
#define SMI_DIRECT_H

#include "PeerToPeer.h"

namespace SMI::Comm {
    class Direct : public PeerToPeer {
    public:
        explicit Direct(std::map<std::string, std::string> params, std::map<std::string, std::string> perf_params);

        void send_object(channel_data buf, Utils::peer_num rcpt_id) override;

        void recv_object(channel_data buf, Utils::peer_num sender_id) override;

        double get_bandwidth(SMI::Utils::peer_num producers, SMI::Utils::peer_num consumers) override;

        double get_overhead() override;

    private:
        std::vector<int> sockets;
        std::string hostname;
        int port;
        unsigned int max_timeout;

        void check_socket(Utils::peer_num partner_id, std::string pair_name);
    };
}



#endif //SMI_DIRECT_H
