#ifndef SMI_CHANNEL_H
#define SMI_CHANNEL_H

#include "Data.h"
#include <string>
#include <map>
#include "../utils/Function.h"
#include "../utils/Common.h"

using raw_function = std::function<char*(char*, char*)>;

struct channel_data {
    char* buf;
    std::size_t len;
};



namespace SMI::Comm {
    class Channel {
    public:
        virtual void send(channel_data buf, SMI::Utils::peer_num dest) = 0;

        virtual void recv(channel_data buf, SMI::Utils::peer_num dest) = 0;

        /*virtual void reduce(channel_data sendbuf, channel_data recvbuf, peer_num root, raw_function f) = 0;

        virtual void allreduce(channel_data sendbuf, channel_data recvbuf, raw_function f) = 0;

        virtual void gather(channel_data sendbuf, channel_data recvbuf, peer_num root) = 0;

        virtual void scatter(channel_data sendbuf, channel_data recvbuf, peer_num root) = 0;

        virtual void bcast(channel_data buf, peer_num root) = 0;

        virtual void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) = 0;*/

        virtual void set_peer_id(SMI::Utils::peer_num num) { peer_id = num; }

        static std::shared_ptr<Channel> get_channel(std::string name, std::map<std::string, std::string> params);

    protected:
        SMI::Utils::peer_num peer_id;

    };

}



#endif //SMI_CHANNEL_H