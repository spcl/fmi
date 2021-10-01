#ifndef SMI_CENTRALCHANNEL_H
#define SMI_CENTRALCHANNEL_H

#include "Channel.h"

namespace SMI::Comm {
    class CentralChannel : public Channel {
    public:
        void send(channel_data buf, peer_num dest) override;

        void recv(channel_data buf, peer_num dest) override;

        virtual void upload(channel_data buf, std::string name) = 0;

        virtual void download(channel_data buf, std::string name) = 0;
    };
}



#endif //SMI_CENTRALCHANNEL_H
