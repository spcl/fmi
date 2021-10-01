#include "../include/Communicator.h"

#include <utility>
namespace SMI {
    Communicator::Communicator(unsigned int peer_id, unsigned int num_peers, std::string config_path, bool sync) {
        Utils::Configuration config(config_path);
    }
}

