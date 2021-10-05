#ifndef SMI_REDIS_H
#define SMI_REDIS_H

#include "ClientServer.h"
#include <map>
#include <string>
#include <hiredis/hiredis.h>

namespace SMI::Comm {

    class Redis : public ClientServer {
    public:
        explicit Redis(std::map<std::string, std::string> params);

        ~Redis();

        void upload_object(channel_data buf, std::string name) override;

        bool download_object(channel_data buf, std::string name) override;

        void delete_object(std::string name) override;

        std::vector<std::string> get_object_names() override;

    private:
        redisContext* context;

    };
}

#endif //SMI_REDIS_H
