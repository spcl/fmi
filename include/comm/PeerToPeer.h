#ifndef SMI_PEERTOPEER_H
#define SMI_PEERTOPEER_H

#include "Channel.h"

namespace SMI::Comm {
    class PeerToPeer : public Channel {
    public:
        void send(channel_data buf, SMI::Utils::peer_num dest) override;

        void recv(channel_data buf, SMI::Utils::peer_num src) override;

        void bcast(channel_data buf, SMI::Utils::peer_num root) override;

        void barrier() override;

        void gather(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) override;

        void scatter(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) override;

        void reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) override;

        void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        virtual void send_object(channel_data buf, Utils::peer_num peer_id) = 0;

        virtual void recv_object(channel_data buf, Utils::peer_num peer_id) = 0;

    protected:
        void reduce_ltr(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, const raw_function& f);

        void reduce_no_order(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, const raw_function& f);

    private:
        Utils::peer_num transform_peer_id(Utils::peer_num id, Utils::peer_num root, bool forward);

    };
}



#endif //SMI_PEERTOPEER_H
