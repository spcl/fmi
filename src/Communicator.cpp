#include "../include/Communicator.h"

#include <utility>
namespace SMI {
    Communicator::Communicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name) {
        Utils::Configuration config(config_path);
        this->peer_id = peer_id;
        this->num_peers = num_peers;
        this->comm_name = comm_name;
        auto backends = config.get_backends();
        for (auto const& [backend_name, backend_params] : backends) {
            if (backend_params.find("enabled")->second == "true") {
                register_channel(backend_name, Comm::Channel::get_channel(backend_name, backend_params));
            }
        }
    }

    void Communicator::register_channel(std::string name, std::shared_ptr<SMI::Comm::Channel> c) {
        c->set_peer_id(peer_id);
        c->set_num_peers(num_peers);
        c->set_comm_name(comm_name);
        channels[name] = c;
    }

    Communicator::~Communicator() {
        for (auto const& [name, channel] : channels) {
            channel->finalize();
        }
    }
}

