#include "../../include/comm/CentralChannel.h"
#include <thread>

void SMI::Comm::CentralChannel::send(channel_data buf, SMI::Utils::peer_num dest) {
    auto num_operation_entry = num_operations.find("send" + std::to_string(dest));
    unsigned int operation_num;
    if (num_operation_entry == num_operations.end()) {
        operation_num = 0;
    } else {
        operation_num = num_operation_entry->second;
    }
    std::string file_name = comm_name + std::to_string(peer_id) + "_" + std::to_string(dest) + "_" + std::to_string(operation_num);
    operation_num++;
    num_operations["send" + std::to_string(dest)] = operation_num;
    upload(buf, file_name);
}

void SMI::Comm::CentralChannel::recv(channel_data buf, SMI::Utils::peer_num dest) {
    auto num_operation_entry = num_operations.find("recv" + std::to_string(dest));
    unsigned int operation_num;
    if (num_operation_entry == num_operations.end()) {
        operation_num = 0;
    } else {
        operation_num = num_operation_entry->second;
    }
    std::string file_name = comm_name + std::to_string(dest) + "_" + std::to_string(peer_id) + "_" + std::to_string(operation_num);
    operation_num++;
    num_operations["recv" + std::to_string(dest)] = operation_num;
    download(buf, file_name);
}

void SMI::Comm::CentralChannel::bcast(channel_data buf, SMI::Utils::peer_num root) {
    std::string file_name = comm_name + std::to_string(root) + "_bcast_" + std::to_string(num_operations["bcast"]);
    num_operations["bcast"]++;
    if (peer_id == root) {
        upload(buf, file_name);
    } else {
        download(buf, file_name);
    }
}

void SMI::Comm::CentralChannel::barrier() {
    auto barrier_num = num_operations["barrier"];
    std::string barrier_suffix = "_barrier_" + std::to_string(barrier_num);
    std::string file_name = comm_name + std::to_string(peer_id) + barrier_suffix;
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
        if (num_arrived >= num_peers) {
            return;
        } else {
            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }
}

void SMI::Comm::CentralChannel::finalize() {
    for (const auto& object_name : created_objects) {
        delete_object(object_name);
    }
}

void SMI::Comm::CentralChannel::download(channel_data buf, std::string name) {
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout) {
        bool success = download_object(buf, name);
        if (success) {
            return;
        } else {
            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }
}

void SMI::Comm::CentralChannel::upload(channel_data buf, std::string name) {
    created_objects.push_back(name);
    upload_object(buf, name);
}
