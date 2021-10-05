#ifndef SMI_PEERTOPEER_H
#define SMI_PEERTOPEER_H

#include "Channel.h"

namespace SMI::Comm {
    class PeerToPeer : public Channel {
    public:
        void send(channel_data buf, SMI::Utils::peer_num dest) override;

        void recv(channel_data buf, SMI::Utils::peer_num dest) override;

        void bcast(channel_data buf, SMI::Utils::peer_num root) override;

        void barrier() override;

        void reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) override;

        void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        virtual void send_object(channel_data buf, Utils::peer_num peer_id) = 0;

        virtual void recv_object(channel_data buf, Utils::peer_num peer_id) = 0;
    };
}



#endif //SMI_PEERTOPEER_H
