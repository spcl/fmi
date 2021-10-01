#ifndef SMI_CENTRALCHANNEL_H
#define SMI_CENTRALCHANNEL_H

#include "Channel.h"

namespace SMI::Comm {
    class CentralChannel : public Channel {
    public:
        template<typename T>
        void send(Data<T> buf, peer_num dest, int tag);

        template<typename T>
        void recv(Data<T> buf, peer_num dest, int tag);

        template<typename T>
        void upload(Data<T> buf, std::string name);

        template<typename T>
        Data<T> download(std::string name);
    };
}



#endif //SMI_CENTRALCHANNEL_H
