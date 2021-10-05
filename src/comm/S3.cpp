#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../../include/comm/S3.h"
#include <boost/log/trivial.hpp>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>

char TAG[] = "S3Client";

SMI::Comm::S3::S3(std::map<std::string, std::string> params) : ClientServer(params) {
    if (instances == 0) {
        // Only one call allowed (https://github.com/aws/aws-sdk-cpp/issues/456), give possible multiple clients control over initialization
        Aws::InitAPI(options);
    }
    instances++;
    bucket_name = params["bucket_name"];
    Aws::Client::ClientConfiguration config;
    config.region = params["s3_region"];

    auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
    client = Aws::MakeUnique<Aws::S3::S3Client>(TAG, credentialsProvider, config);
}

SMI::Comm::S3::~S3() {
    instances--;
    if (instances == 0) {
        Aws::ShutdownAPI(options);
    }
}

bool SMI::Comm::S3::download_object(channel_data buf, std::string name) {
    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    auto outcome = client->GetObject(request);
    if (outcome.IsSuccess()) {
        auto& s = outcome.GetResult().GetBody();
        s.read(buf.buf, buf.len);
        return true;
    } else {
        return false;
    }
}

void SMI::Comm::S3::upload_object(channel_data buf, std::string name) {
    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);

    const std::shared_ptr<Aws::IOStream> data = Aws::MakeShared<boost::interprocess::bufferstream>(TAG, buf.buf, buf.len);

    request.SetBody(data);
    auto outcome = client->PutObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when uploading to S3: " << outcome.GetError();
    }
}

void SMI::Comm::S3::delete_object(std::string name) {
    Aws::S3::Model::DeleteObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    auto outcome = client->DeleteObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when deleting from S3: " << outcome.GetError();
    }
}

std::vector<std::string> SMI::Comm::S3::get_object_names() {
    std::vector<std::string> object_names;
    Aws::S3::Model::ListObjectsRequest request;
    request.WithBucket(bucket_name);
    auto outcome = client->ListObjects(request);
    if (outcome.IsSuccess()) {
        auto objects = outcome.GetResult().GetContents();
        for (auto& object : objects) {
            object_names.push_back(object.GetKey());
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error when listing objects from S3: " << outcome.GetError();
    }
    return object_names;
}

