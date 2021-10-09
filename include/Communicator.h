#ifndef SMI_COMMUNICATOR_H
#define SMI_COMMUNICATOR_H

#include "./utils/Configuration.h"
#include "comm/Channel.h"
#include "utils/ChannelPolicy.h"

namespace SMI {
    class Communicator {
    public:
        Communicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name,
                     unsigned int faas_memory = 128);

        ~Communicator();

        template<typename T>
        void send(Comm::Data<T> &buf, SMI::Utils::peer_num dest) {
            std::string channel = policy->get_channel({Utils::send, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->send(data, dest);
        }

        template<typename T>
        void recv(Comm::Data<T> &buf, SMI::Utils::peer_num src) {
            std::string channel = policy->get_channel({Utils::send, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->recv(data, src);
        }

        template<typename T>
        void bcast(Comm::Data<T> &buf, SMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::bcast, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->bcast(data, root);
        }

        void barrier() {
            std::string channel = policy->get_channel({Utils::barrier, 0});
            channels[channel]->barrier();
        }

        template<typename T>
        void gather(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::gather, sendbuf.size_in_bytes()});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels[channel]->gather(senddata, recvdata, root);
        }

        template<typename T>
        void scatter(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::scatter, recvbuf.size_in_bytes()});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels[channel]->scatter(senddata, recvdata, root);
        }

        template <typename T>
        void reduce(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::peer_num root, SMI::Utils::Function<T> f) {
            if (peer_id == root && sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw "Dimensions of send and receive data must match";
            }
            bool left_to_right = !(f.commutative && f.associative);
            std::string channel = policy->get_channel({Utils::reduce, sendbuf.size_in_bytes(), left_to_right});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            auto func = convert_to_raw_function(f, sendbuf.size_in_bytes());
            raw_function raw_f {
                func,
                f.associative,
                f.commutative
            };
            channels[channel]->reduce(senddata, recvdata, root, raw_f);
        }

        template <typename T>
        void allreduce(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::Function<T> f) {
            if (sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw "Dimensions of send and receive data must match";
            }
            bool left_to_right = !(f.commutative && f.associative);
            std::string channel = policy->get_channel({Utils::allreduce, sendbuf.size_in_bytes(), left_to_right});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            auto func = convert_to_raw_function(f, sendbuf.size_in_bytes());
            raw_function raw_f {
                func,
                f.associative,
                f.commutative
            };
            channels[channel]->allreduce(senddata, recvdata, raw_f);
        }

        template<typename T>
        void scan(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, SMI::Utils::Function<T> f) {
            if (sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw "Dimensions of send and receive data must match";
            }
            std::string channel = policy->get_channel({Utils::scan, sendbuf.size_in_bytes()});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            auto func = convert_to_raw_function(f, sendbuf.size_in_bytes());
            raw_function raw_f {
                func,
                f.associative,
                f.commutative
            };
            channels[channel]->scan(senddata, recvdata, raw_f);
        }

        void register_channel(std::string name, std::shared_ptr<SMI::Comm::Channel>);

        void set_channel_policy(std::shared_ptr<SMI::Utils::ChannelPolicy> policy);

        void set_hint(SMI::Utils::Hint hint);

    private:
        std::shared_ptr<SMI::Utils::ChannelPolicy> policy;
        std::map<std::string, std::shared_ptr<SMI::Comm::Channel>> channels;
        SMI::Utils::peer_num peer_id;
        SMI::Utils::peer_num num_peers;
        std::string comm_name;
        SMI::Utils::Hint hint = SMI::Utils::Hint::cheap;

        template <typename T>
        raw_func convert_to_raw_function(SMI::Utils::Function<T> f, std::size_t size_in_bytes) {
            auto func = [f](char* a, char* b) -> void {
                T* dest = reinterpret_cast<T*>(a);
                *dest = f(*((T*) a), *((T*) b));
            };
            return func;
        }

        template <typename A>
        raw_func convert_to_raw_function(SMI::Utils::Function<std::vector<A>> f, std::size_t size_in_bytes) {
            auto func = [f, size_in_bytes](char* a, char* b) -> void {
                std::vector<A> vec_a((A*) a, (A*) (a + size_in_bytes));
                std::vector<A> vec_b((A*) b, (A*) (b + size_in_bytes));
                std::vector<A> res = f(vec_a, vec_b);
                std::memcpy(a, (char*) res.data(), size_in_bytes);
            };
            return func;
        }
    };
}



#endif //SMI_COMMUNICATOR_H
