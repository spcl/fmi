#include "../../include/comm/Channel.h"
#include "../../include/comm/S3.h"

std::shared_ptr<SMI::Comm::Channel> SMI::Comm::Channel::get_channel(std::string name, std::map<std::string, std::string> params) {
    if (name == "S3") {
        return std::make_shared<S3>(params);
    }
}
