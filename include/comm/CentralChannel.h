#ifndef SMI_CENTRALCHANNEL_H
#define SMI_CENTRALCHANNEL_H

#include "Channel.h"
#include "../utils/Common.h"

namespace SMI::Comm {
    class CentralChannel : public Channel {
    public:
        void send(channel_data buf, SMI::Utils::peer_num dest) override;

        void recv(channel_data buf, SMI::Utils::peer_num dest) override;

        void bcast(channel_data buf, SMI::Utils::peer_num root) override;

        void barrier() override;

        void reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) override;

        void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        virtual void upload_object(channel_data buf, std::string name) = 0;

        virtual bool download_object(channel_data buf, std::string name) = 0;

        virtual void download(channel_data buf, std::string name);

        virtual void upload(channel_data buf, std::string name);

        virtual std::vector<std::string> get_object_names() = 0;

        virtual void delete_object(std::string name) = 0;

        void finalize() override;

    protected:
        std::map<std::string, unsigned int> num_operations = {
                {"bcast", 0},
                {"barrier", 0},
                {"reduce", 0},
                {"scan", 0}
        };
        std::vector<std::string> created_objects;
        unsigned int timeout;
        unsigned int max_timeout;
    };
}



#endif //SMI_CENTRALCHANNEL_H
