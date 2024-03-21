#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../../include/comm/S3.h"
#include <boost/log/trivial.hpp>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <cmath>

char TAG[] = "S3Client";

FMI::Comm::S3::S3(std::map<std::string, std::string> params, std::map<std::string, std::string> model_params) : ClientServer(params) {
    if (instances == 0) {
        // Only one call allowed (https://github.com/aws/aws-sdk-cpp/issues/456), give possible multiple clients control over initialization
        Aws::InitAPI(options);
    }
    instances++;
    bucket_name = params["bucket_name"];
    Aws::Client::ClientConfiguration config;
    config.region = params["s3_region"];
    bandwidth = std::stod(model_params["bandwidth"]);
    overhead = std::stod(model_params["overhead"]);
    transfer_price = std::stod(model_params["transfer_price"]);
    download_price = std::stod(model_params["download_price"]);
    upload_price = std::stod(model_params["upload_price"]);

    client = Aws::MakeUnique<Aws::S3::S3Client>(TAG, config);
}

FMI::Comm::S3::~S3() {
    instances--;
    if (instances == 0) {
        Aws::ShutdownAPI(options);
    }
}

bool FMI::Comm::S3::download_object(channel_data buf, std::string name) {
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

void FMI::Comm::S3::upload_object(channel_data buf, std::string name) {
    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);

    const std::shared_ptr<Aws::IOStream> data = Aws::MakeShared<boost::interprocess::bufferstream>(TAG, buf.buf, buf.len);

    request.SetBody(data);
    auto outcome = client->PutObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when uploading to S3: " << outcome.GetError();
    }
}

void FMI::Comm::S3::delete_object(std::string name) {
    Aws::S3::Model::DeleteObjectRequest request;
    request.WithBucket(bucket_name).WithKey(name);
    auto outcome = client->DeleteObject(request);
    if (!outcome.IsSuccess()) {
        BOOST_LOG_TRIVIAL(error) << "Error when deleting from S3: " << outcome.GetError();
    }
}

std::vector<std::string> FMI::Comm::S3::get_object_names() {
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

double FMI::Comm::S3::get_latency(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double fixed_overhead = overhead;
    double waiting_time = (double) timeout / 2.;
    double comm_overhead = fixed_overhead + waiting_time;
    double agg_bandwidth = producer * consumer * bandwidth;
    double trans_time = producer * consumer * ((double) size_in_bytes / 1000000.) / agg_bandwidth;
    return log2(producer + consumer) * comm_overhead + trans_time;
}

double FMI::Comm::S3::get_price(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double upload_costs = producer * upload_price + producer * ((double) size_in_bytes / 1000000000.) * transfer_price;
    double expected_polls = (max_timeout / timeout) / 2;
    double download_costs = producer * consumer * expected_polls * download_price + producer * consumer * ((double) size_in_bytes / 1000000000.) * transfer_price;
    return upload_costs + download_costs;
}

