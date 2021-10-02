#include "../../include/comm/CentralChannel.h"
#include <iostream>

void SMI::Comm::CentralChannel::send(channel_data buf, SMI::Utils::peer_num dest) {
    std::string file_name = std::to_string(peer_id) + "_" + std::to_string(dest);
    upload(buf, file_name);
}

void SMI::Comm::CentralChannel::recv(channel_data buf, SMI::Utils::peer_num dest) {
    std::string file_name = std::to_string(dest) + "_" + std::to_string(peer_id);
    download(buf, file_name, true);
}
