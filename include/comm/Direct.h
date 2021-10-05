#ifndef SMI_DIRECT_H
#define SMI_DIRECT_H

#include "PeerToPeer.h"

namespace SMI::Comm {
    class Direct : public PeerToPeer {
    public:
        explicit Direct(std::map<std::string, std::string> params);

        void send_object(channel_data buf, Utils::peer_num rcpt_id) override;

        void recv_object(channel_data buf, Utils::peer_num sender_id) override;

    private:
        std::vector<int> sockets;
        std::string hostname;
        int port;
    };
}



#endif //SMI_DIRECT_H
