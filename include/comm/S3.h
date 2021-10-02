#ifndef SMI_S3_H
#define SMI_S3_H

#include "CentralChannel.h"
#include <map>
#include <string>
#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include <boost/interprocess/streams/bufferstream.hpp>

namespace SMI::Comm {
    class S3 : public CentralChannel {
    public:
        explicit S3(std::map<std::string, std::string> params, bool init_api = true);

        ~S3();

        void upload(channel_data buf, std::string name) override;

        void download(channel_data buf, std::string name, bool cleanup) override;

        std::vector<std::string> get_object_names() override;

    private:
        std::string bucket_name;
        std::unique_ptr<Aws::S3::S3Client, Aws::Deleter<Aws::S3::S3Client>> client;
        Aws::SDKOptions options;
        bool has_initialized = false;

        void delete_object(std::string name) override;
    };
}


#endif //SMI_S3_H
