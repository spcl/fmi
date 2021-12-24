#ifndef FMI_COMMUNICATOR_H
#define FMI_COMMUNICATOR_H

#include "./utils/Configuration.h"
#include "comm/Channel.h"
#include "utils/ChannelPolicy.h"

namespace FMI {
    //! Interface that is exposed to the user for interaction with the FMI system.
    class Communicator {
    public:
        /*!
         * @param peer_id ID of the peer in the range [0 .. num_peers - 1]
         * @param num_peers Number of peers participating in the communicator
         * @param config_path Path to the FMI JSON configuration file
         * @param comm_name Name of the communicator, needs to be unique when multiple communicators are used concurrently
         * @param faas_memory Amount of memory (in MiB) that is allocated to the function, used for performance model calculations.
         */
        Communicator(FMI::Utils::peer_num peer_id, FMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name,
                     unsigned int faas_memory = 128);

        //! Finalizes all active channels
        ~Communicator();

        //! Send buf to peer dest
        template<typename T>
        void send(Comm::Data<T> &buf, FMI::Utils::peer_num dest) {
            std::string channel = policy->get_channel({Utils::send, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->send(data, dest);
        }

        //! Receive data from src and store data into the provided buf
        template<typename T>
        void recv(Comm::Data<T> &buf, FMI::Utils::peer_num src) {
            std::string channel = policy->get_channel({Utils::send, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->recv(data, src);
        }

        //! Broadcast the data that is in the provided buf of the root peer. Result is stored in buf for all peers.
        template<typename T>
        void bcast(Comm::Data<T> &buf, FMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::bcast, buf.size_in_bytes()});
            channel_data data {buf.data(), buf.size_in_bytes()};
            channels[channel]->bcast(data, root);
        }

        //! Barrier synchronization collective
        void barrier() {
            std::string channel = policy->get_channel({Utils::barrier, 0});
            channels[channel]->barrier();
        }

        //! Gather the data of the individuals peers (in sendbuf) into the recvbuf of root.
        /*!
         * @param sendbuf Data to send to root, needs to be the same size for all peers.
         * @param recvbuf Receive buffer, only relevant for the root process. Size needs to be num_peers * sendbuf.size
         */
        template<typename T>
        void gather(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, FMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::gather, sendbuf.size_in_bytes()});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels[channel]->gather(senddata, recvdata, root);
        }

        //! Scatter the data from root's sendbuf to the recvbuf of all peers.
        /*!
         * @param sendbuf The data to scatter, size needs to be recvbuf.size * num_peers (i.e., divisible by the number of peers). Only relevant for the root peer.
         * @param recvbuf Buffer to receive the data, relevant for all peers.
         */
        template<typename T>
        void scatter(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, FMI::Utils::peer_num root) {
            std::string channel = policy->get_channel({Utils::scatter, recvbuf.size_in_bytes()});
            channel_data senddata {sendbuf.data(), sendbuf.size_in_bytes()};
            channel_data recvdata {recvbuf.data(), recvbuf.size_in_bytes()};
            channels[channel]->scatter(senddata, recvdata, root);
        }

        //! Perform a reduction with the reduction function f.
        /*! Depending on the associativity / commutativity of f, a different implementation for the reduction may be used.
         * However, in the same topology, the evaluation order should always be the same, irrespectively of the associativity / commutativitiy.
         * @param sendbuf Data to send, relevant for all peers.
         * @param recvbuf Receive buffer that contains the final result, only relevant for root. Needs to have the same size as the sendbuf.
         */
        template <typename T>
        void reduce(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, FMI::Utils::peer_num root, FMI::Utils::Function<T> f) {
            if (peer_id == root && sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw std::runtime_error("Dimensions of send and receive data must match");
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

        //! Perform a reduction with the reduction function f and make the result available to all peers.
        /*! Depending on the associativity / commutativity of f, a different implementation for the reduction may be used.
         * However, in the same topology, the evaluation order should always be the same, irrespectively of the associativity / commutativitiy.
         * @param sendbuf Data to send, relevant for all peers.
         * @param recvbuf Receive buffer that contains the final result, relevant for all peers. Needs to have the same size as the sendbuf.
         */
        template <typename T>
        void allreduce(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, FMI::Utils::Function<T> f) {
            if (sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw std::runtime_error("Dimensions of send and receive data must match");
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

        //! Inclusive prefix scan.
        /*! Depending on the associativity / commutativity of f, a different implementation for the reduction may be used.
         * However, in the same topology, the evaluation order should always be the same, irrespectively of the associativity / commutativitiy.
         * @param sendbuf Data to send, relevant for all peers.
         * @param recvbuf Receive buffer that contains the final result, relevant for all peers. Needs to have the same size as the sendbuf.
         */
        template<typename T>
        void scan(Comm::Data<T> &sendbuf, Comm::Data<T> &recvbuf, FMI::Utils::Function<T> f) {
            if (sendbuf.size_in_bytes() != recvbuf.size_in_bytes()) {
                throw std::runtime_error("Dimensions of send and receive data must match");
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

        //! Add a new channel to the communicator with the given name by providing a pointer to it.
        void register_channel(std::string name, std::shared_ptr<FMI::Comm::Channel>);

        //! Change the channel policy the communicator is using.
        void set_channel_policy(std::shared_ptr<FMI::Utils::ChannelPolicy> policy);

        //! Set the hint (optimization objective) of the channel selection procedure.
        void hint(FMI::Utils::Hint hint);

    private:
        std::shared_ptr<FMI::Utils::ChannelPolicy> policy;
        std::map<std::string, std::shared_ptr<FMI::Comm::Channel>> channels;
        FMI::Utils::peer_num peer_id;
        FMI::Utils::peer_num num_peers;
        std::string comm_name;
        FMI::Utils::Hint channel_hint = FMI::Utils::Hint::cheap;

        //! Helper utility to convert a typed function to a raw function without type information.
        template <typename T>
        raw_func convert_to_raw_function(FMI::Utils::Function<T> f, std::size_t size_in_bytes) {
            auto func = [f](char* a, char* b) -> void {
                T* dest = reinterpret_cast<T*>(a);
                *dest = f(*((T*) a), *((T*) b));
            };
            return func;
        }

        //! Helper utility to convert a vector function to a raw function that operates directly on memory pointers.
        template <typename A>
        raw_func convert_to_raw_function(FMI::Utils::Function<std::vector<A>> f, std::size_t size_in_bytes) {
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



#endif //FMI_COMMUNICATOR_H
