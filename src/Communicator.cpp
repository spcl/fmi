#include "../include/Communicator.h"

#include <utility>
namespace SMI {
    Communicator::Communicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path, bool sync) {
        Utils::Configuration config(config_path);
        peer_id = peer_id;
        num_peers = num_peers;
        auto backends = config.get_backends();
        for (auto const& [backend_name, backend_params] : backends) {
            if (backend_params.find("enabled")->second == "true") {
                register_channel(backend_name, Comm::Channel::get_channel(backend_name, backend_params));
            }
        }
    }

    void Communicator::register_channel(std::string name, std::shared_ptr<SMI::Comm::Channel> c) {
        c->set_peer_id(peer_id);
        channels[name] = c;
    }
}

