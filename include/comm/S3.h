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
        S3(std::map<std::string, std::any> params);

        ~S3();

        template<typename T>
        void upload(SMI::Comm::Data<T> buf, std::string name) {
            Aws::S3::Model::PutObjectRequest request;
            request.WithBucket(bucket_name).WithKey(name);

            const std::shared_ptr<Aws::IOStream> data = std::make_shared<boost::interprocess::bufferstream>(buf.data(),
                                                                                                           buf.size_in_bytes());

            request.SetBody(data);
            Aws::S3::Model::PutObjectOutcome outcome = client->PutObject(request);
            if (!outcome.IsSuccess()) {
                std::cout << "Error: PutObject: " <<
                outcome.GetError() << std::endl;
            }
        }

        template<typename T>
        void download(Data<T> &buf, std::string name) {
            Aws::S3::Model::GetObjectRequest request;
            request.WithBucket(bucket_name).WithKey(name);

            auto outcome = client->GetObject(request);
            if (outcome.IsSuccess()) {
                auto& s = outcome.GetResult().GetBody();
                s.read(buf.data(), buf.size_in_bytes());
            }
        }
    private:
        std::string bucket_name;
        std::unique_ptr<Aws::S3::S3Client> client;
        Aws::SDKOptions options;
    };
}


#endif //SMI_S3_H
