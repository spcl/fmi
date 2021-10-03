#ifndef SMI_COMMUNICATOR_H
#define SMI_COMMUNICATOR_H

#include "./utils/Configuration.h"
#include "comm/Channel.h"

namespace SMI {
    class Communicator {
    public:
        Communicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name);

        ~Communicator();

        template<typename T>
        void send(Comm::Data<T> &buf, SMI::Utils::peer_num dest) {
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels["S3"]->send(data, dest);
        }

        template<typename T>
        void recv(Comm::Data<T> &buf, SMI::Utils::peer_num src) {
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels["S3"]->recv(data, src);
        }

        template<typename T>
        void bcast(Comm::Data<T> &buf, SMI::Utils::peer_num root) {
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels["S3"]->bcast(data, root);
        }

        template<typename T>
        void gather(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::peer_num root) {
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels["S3"]->gather(senddata, recvdata, root);
        }

        template<typename T>
        void scatter(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::peer_num root) {
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels["S3"]->scatter(senddata, recvdata, root);
        }

        template <typename T>
        void reduce(Comm::Data<T> sendbuf, Comm::Data<T> recvbuf, SMI::Utils::peer_num root, SMI::Utils::Function<T> f);

        template <typename T>
        void allreduce(Comm::Data<T> sendbuf, Comm::Data<T> recvbuf, SMI::Utils::Function<T> f);

        template<typename T>
        void scan(Comm::Data<T> sendbuf, Comm::Data<T> recvbuf, SMI::Utils::Function<T> f);

        void register_channel(std::string name, std::shared_ptr<SMI::Comm::Channel>);

    private:
        std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> channels;
        SMI::Utils::peer_num peer_id;
        SMI::Utils::peer_num num_peers;
        std::string comm_name;
    };
}



#endif //SMI_COMMUNICATOR_H
