#ifndef SMI_S3_H
#define SMI_S3_H

#include "ClientServer.h"
#include <map>
#include <string>
#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include <boost/interprocess/streams/bufferstream.hpp>

namespace SMI::Comm {
    class S3 : public ClientServer {
    public:
        explicit S3(std::map<std::string, std::string> params);

        ~S3();

        void upload_object(channel_data buf, std::string name) override;

        bool download_object(channel_data buf, std::string name) override;

        void delete_object(std::string name) override;

        std::vector<std::string> get_object_names() override;

    private:
        std::string bucket_name;
        std::unique_ptr<Aws::S3::S3Client, Aws::Deleter<Aws::S3::S3Client>> client;
        Aws::SDKOptions options;
        inline static int instances = 0;

    };
}


#endif //SMI_S3_H
