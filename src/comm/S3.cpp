#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../../include/comm/S3.h"
#include <boost/log/trivial.hpp>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>

char TAG[] = "S3Client";

SMI::Comm::S3::S3(std::map<std::string, std::string> params) {
    Aws::InitAPI(options);
    bucket_name = params["bucket_name"];
    Aws::Client::ClientConfiguration config;
    config.region = params["s3_region"];
    timeout = std::stoi(params["timeout"]);
    max_timeout = std::stoi(params["max_timeout"]);

    auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
    client = Aws::MakeUnique<Aws::S3::S3Client>(TAG, credentialsProvider, config);
}

SMI::Comm::S3::~S3() {
    Aws::ShutdownAPI(options);
}

void SMI::Comm::S3::download(channel_data buf, std::string name, bool cleanup) {
    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout) {
        auto outcome = client->GetObject(request);
        if (outcome.IsSuccess()) {
            auto& s = outcome.GetResult().GetBody();
            s.read(buf.buf, buf.len);
            if (cleanup) {
                delete_file(name);
            }
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

    const std::shared_ptr<Aws::IOStream> data = Aws::MakeShared<boost::interprocess::bufferstream>(TAG, buf.buf,
                                                                                                    buf.len);

    request.SetBody(data);
    auto outcome = client->PutObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when uploading to S3: " << outcome.GetError();
    }
}

void SMI::Comm::S3::delete_file(std::string name) {
    Aws::S3::Model::DeleteObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    auto outcome = client->DeleteObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when deleting from S3: " << outcome.GetError();
    }
}

