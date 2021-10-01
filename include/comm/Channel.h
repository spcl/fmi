#ifndef SMI_CHANNEL_H
#define SMI_CHANNEL_H

#include "Data.h"
#include "../utils/Function.h"

using peer_num = unsigned int;

namespace SMI::Comm {
    class Channel {
    public:
        template<typename T>
        void send(Data<T> buf, peer_num dest, int tag);

        template<typename T>
        void recv(Data<T> buf, peer_num dest, int tag);

        template <typename T>
        void reduce(Data<T> sendbuf, Data<T> recvbuf, peer_num root, SMI::Utils::Function<T> f);

        template <typename T>
        void allreduce(Data<T> sendbuf, Data<T> recvbuf, SMI::Utils::Function<T> f);

        template<typename T>
        void gather(Data<T> sendbuf, Data<T> recvbuf, peer_num root);

        template<typename T>
        void scatter(Data<T> sendbuf, Data<T> recvbuf, peer_num root);

        template<typename T>
        void bcast(Data<T> buf, peer_num root);

        template<typename T>
        void scan(Data<T> sendbuf, Data<T> recvbuf, SMI::Utils::Function<T> f);
    };
}



#endif //SMI_CHANNEL_H
