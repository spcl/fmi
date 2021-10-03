#ifndef SMI_CHANNEL_H
#define SMI_CHANNEL_H

#include "Data.h"
#include <string>
#include <map>
#include "../utils/Function.h"
#include "../utils/Common.h"

struct raw_function {
    std::function<void(char*, char*)> f; // Overwrites left arg.
    bool associative;
    bool commutative;
};

struct channel_data {
    char* buf;
    std::size_t len;
};



namespace SMI::Comm {
    class Channel {
    public:
        virtual void send(channel_data buf, SMI::Utils::peer_num dest) = 0;

        virtual void recv(channel_data buf, SMI::Utils::peer_num dest) = 0;

        virtual void bcast(channel_data buf, SMI::Utils::peer_num root) = 0;

        virtual void barrier() = 0;

        virtual void gather(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root);

        virtual void scatter(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root);

        virtual void reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) = 0;

        virtual void allreduce(channel_data sendbuf, channel_data recvbuf, raw_function f);
        /*;


        virtual void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) = 0;*/

        void set_peer_id(SMI::Utils::peer_num num) { peer_id = num; }

        void set_num_peers(SMI::Utils::peer_num num) { num_peers = num; }

        void set_comm_name(std::string communication_name) {comm_name = communication_name; }

        virtual void finalize() = 0;

        static std::shared_ptr<Channel> get_channel(std::string name, std::map<std::string, std::string> params);

    protected:
        SMI::Utils::peer_num peer_id;
        SMI::Utils::peer_num num_peers;
        std::string comm_name;

    };

}



#endif //SMI_CHANNEL_H
