#include "../include/Communicator.h"

#include <utility>
namespace FMI {
    Communicator::Communicator(FMI::Utils::peer_num peer_id, FMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name,
                               unsigned int faas_memory) {
        Utils::Configuration config(config_path);
        this->peer_id = peer_id;
        this->num_peers = num_peers;
        this->comm_name = comm_name;
        auto backends = config.get_active_channels();
        for (auto const& [backend_name, params] : backends) {
            auto backend_params = params.first;
            auto model_params = params.second;
            if (backend_params.find("enabled")->second == "true") {
                register_channel(backend_name, Comm::Channel::get_channel(backend_name, backend_params, model_params));
            }
        }
        double gib_second_price = config.get_faas_price();
        double faas_price = (double) faas_memory / 1024. * gib_second_price;
        set_channel_policy(std::make_shared<FMI::Utils::ChannelPolicy>(channels, num_peers, faas_price, channel_hint));
    }

    void Communicator::register_channel(std::string name, std::shared_ptr<FMI::Comm::Channel> c) {
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

    void Communicator::set_channel_policy(std::shared_ptr<FMI::Utils::ChannelPolicy> policy) {
        this->policy = std::move(policy);
    }

    void Communicator::hint(FMI::Utils::Hint hint) {
        this->channel_hint = hint;
        policy->set_hint(hint);
    }

}
