#include "../../include/comm/CentralChannel.h"
#include <iostream>
#include <thread>

void SMI::Comm::CentralChannel::send(channel_data buf, SMI::Utils::peer_num dest) {
    std::string file_name = std::to_string(peer_id) + "_" + std::to_string(dest);
    upload(buf, file_name);
}

void SMI::Comm::CentralChannel::recv(channel_data buf, SMI::Utils::peer_num dest) {
    std::string file_name = std::to_string(dest) + "_" + std::to_string(peer_id);
    download(buf, file_name, true);
}

void SMI::Comm::CentralChannel::bcast(channel_data buf, SMI::Utils::peer_num root) {
    std::string file_name = std::to_string(root) + "_bcast_" + std::to_string(num_operations["bcast"]);
    num_operations["bcast"]++;
    if (peer_id == root) {
        upload(buf, file_name);
    } else {
        download(buf, file_name, false);
    }
}

void SMI::Comm::CentralChannel::barrier() {
    auto barrier_num = num_operations["barrier"];
    std::string barrier_suffix = "_barrier_" + std::to_string(barrier_num);
    std::string file_name = std::to_string(peer_id) + barrier_suffix;
    num_operations["barrier"]++;
    char b = '1';
    upload({&b, sizeof(b)}, file_name);
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout) {
        auto objects = get_object_names();
        int num_arrived = 0;
        for (const auto& object_name : objects) {
            if (object_name.size() > barrier_suffix.size() &&
            object_name.compare(object_name.size() - barrier_suffix.size(), barrier_suffix.size(), barrier_suffix) == 0) {
                num_arrived++;
            }
        }
        if (num_arrived == num_peers) {
            return;
        } else {
            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }
}

void SMI::Comm::CentralChannel::finalize() {

}
