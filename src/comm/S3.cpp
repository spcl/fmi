#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../../include/comm/S3.h"


SMI::Comm::S3::S3(std::map<std::string, std::any> params) {
    Aws::InitAPI(options);
    bucket_name = std::any_cast<std::string>(params["bucket_name"]);
    Aws::Client::ClientConfiguration config;
    config.region = std::any_cast<std::string>(params["s3_region"]);

    auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>("CredProvider");
    client = std::make_unique<Aws::S3::S3Client>(credentialsProvider, config);
}

SMI::Comm::S3::~S3() {
    Aws::ShutdownAPI(options);
}


