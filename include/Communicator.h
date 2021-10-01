#ifndef SMI_COMMUNICATOR_H
#define SMI_COMMUNICATOR_H

#include "./utils/Configuration.h"

namespace SMI {
    class Communicator {
    public:
        Communicator(unsigned int peer_id, unsigned int num_peers, std::string config_path, bool sync = false);
    };
}



#endif //SMI_COMMUNICATOR_H
