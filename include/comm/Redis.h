#ifndef FMI_REDIS_H
#define FMI_REDIS_H

#include "ClientServer.h"
#include <map>
#include <string>
#include <hiredis/hiredis.h>

namespace FMI::Comm {
    //! Channel that uses Redis with the Hiredis client library as storage backend.
    class Redis : public ClientServer {
    public:
        explicit Redis(std::map<std::string, std::string> params, std::map<std::string, std::string> model_params);

        ~Redis();

        void upload_object(channel_data buf, std::string name) override;

        bool download_object(channel_data buf, std::string name) override;

        void delete_object(std::string name) override;

        std::vector<std::string> get_object_names() override;

        double get_latency(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) override;

        double get_price(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) override;

    private:
        redisContext* context;
        // Model params
        double bandwidth_single;
        double bandwidth_multiple;
        double overhead;
        double transfer_price;
        double instance_price;
        unsigned int requests_per_hour;
        bool include_infrastructure_costs;

    };
}

#endif //FMI_REDIS_H
