#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../../include/comm/S3.h"
#include <boost/log/trivial.hpp>


SMI::Comm::S3::S3(std::map<std::string, std::string> params) {
    Aws::InitAPI(options);
    bucket_name = params["bucket_name"];
    Aws::Client::ClientConfiguration config;
    config.region = params["s3_region"];
    timeout = std::stoi(params["timeout"]);
    max_timeout = std::stoi(params["max_timeout"]);

    auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>("CredProvider");
    client = std::make_unique<Aws::S3::S3Client>(credentialsProvider, config);
}

SMI::Comm::S3::~S3() {
    Aws::ShutdownAPI(options);
}

void SMI::Comm::S3::download(channel_data buf, std::string name) {
    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout) {
        auto outcome = client->GetObject(request);
        if (outcome.IsSuccess()) {
            auto& s = outcome.GetResult().GetBody();
            s.read(buf.buf, buf.len);
            return;
        } else {
            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }

}

void SMI::Comm::S3::upload(channel_data buf, std::string name) {
    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);

    const std::shared_ptr<Aws::IOStream> data = std::make_shared<boost::interprocess::bufferstream>(buf.buf,
                                                                                                    buf.len);

    request.SetBody(data);
    Aws::S3::Model::PutObjectOutcome outcome = client->PutObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << outcome.GetError();
    }
}

