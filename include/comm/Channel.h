#ifndef SMI_CHANNEL_H
#define SMI_CHANNEL_H

#include "Data.h"
#include <string>
#include <map>
#include <memory>
#include "../utils/Function.h"
#include "../utils/Common.h"

using raw_func = std::function<void(char*, char*)>;

struct raw_function {
    raw_func f; // Overwrites left arg.
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

        virtual void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) = 0;

        void set_peer_id(SMI::Utils::peer_num num) { peer_id = num; }

        void set_num_peers(SMI::Utils::peer_num num) { num_peers = num; }

        void set_comm_name(std::string communication_name) {comm_name = communication_name; }

        virtual void finalize() {};

        static std::shared_ptr<Channel> get_channel(std::string name, std::map<std::string, std::string> params, std::map<std::string, std::string> perf_params);

        virtual double get_bandwidth(SMI::Utils::peer_num producers, SMI::Utils::peer_num consumers) = 0;

        virtual double get_overhead() = 0;

    protected:
        SMI::Utils::peer_num peer_id;
        SMI::Utils::peer_num num_peers;
        std::string comm_name;

    };

}



#endif //SMI_CHANNEL_H
