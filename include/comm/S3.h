#ifndef SMI_S3_H
#define SMI_S3_H

#include "CentralChannel.h"
#include <map>
#include <string>
#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

namespace SMI::Comm {
    class S3 : public CentralChannel {
    public:
        explicit S3(std::map<std::string, std::any> params);

        ~S3();

        void upload(channel_data buf, std::string name) override;

        void download(channel_data buf, std::string name) override;

    private:
        std::string bucket_name;
        std::unique_ptr<Aws::S3::S3Client> client;
        Aws::SDKOptions options;
    };
}


#endif //SMI_S3_H
