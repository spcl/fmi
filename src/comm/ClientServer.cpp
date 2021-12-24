#include "../../include/comm/ClientServer.h"
#include <thread>
#include <cstring>
#include <cmath>

void FMI::Comm::ClientServer::send(channel_data buf, FMI::Utils::peer_num dest) {
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

void FMI::Comm::ClientServer::recv(channel_data buf, FMI::Utils::peer_num dest) {
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

void FMI::Comm::ClientServer::bcast(channel_data buf, FMI::Utils::peer_num root) {
    std::string file_name = comm_name + std::to_string(root) + "_bcast_" + std::to_string(num_operations["bcast"]);
    num_operations["bcast"]++;
    if (peer_id == root) {
        upload(buf, file_name);
    } else {
        download(buf, file_name);
    }
}

void FMI::Comm::ClientServer::barrier() {
    auto barrier_num = num_operations["barrier"];
    std::string barrier_suffix = "_barrier_" + std::to_string(barrier_num);
    std::string file_name = comm_name + std::to_string(peer_id) + barrier_suffix;
    num_operations["barrier"]++;
    char b = '1';
    upload({&b, sizeof(b)}, file_name);
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout) {
        auto objects = get_object_names();
        auto has_barrier_suffix = [barrier_suffix] (const std::string& s){return s.size() > barrier_suffix.size() &&
                                                    s.compare(s.size() - barrier_suffix.size(), barrier_suffix.size(), barrier_suffix) == 0 ;};
        auto num_arrived = std::count_if(objects.begin(), objects.end(), has_barrier_suffix);
        if (num_arrived >= num_peers) {
            return;
        } else {
            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }
    throw Utils::Timeout();
}

void FMI::Comm::ClientServer::finalize() {
    for (const auto& object_name : created_objects) {
        delete_object(object_name);
    }
}

void FMI::Comm::ClientServer::download(channel_data buf, std::string name) {
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
    throw Utils::Timeout();
}

void FMI::Comm::ClientServer::upload(channel_data buf, std::string name) {
    created_objects.push_back(name);
    upload_object(buf, name);
}

void FMI::Comm::ClientServer::reduce(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root, raw_function f) {
    if (peer_id == root) {
        bool left_to_right = !(f.commutative && f.associative);
        std::vector<bool> received(num_peers, false);
        std::vector<bool> applied(num_peers, false);
        auto buffer_length = sendbuf.len;
        std::vector<char> data(buffer_length * num_peers);
        std::memcpy(reinterpret_cast<void*>(recvbuf.buf), sendbuf.buf, buffer_length);
        received[root] = true;
        applied[root] = true;
        unsigned int elapsed_time = 0;
        while (elapsed_time < max_timeout && std::any_of(applied.begin(), applied.end(), [] (bool v) { return !v; }) ) {
            // Receive all values
            for (int i = 0; i < num_peers; i++) {
                if (received[i]) {
                    continue;
                }
                std::string file_name = comm_name + std::to_string(i) + "_reduce_" + std::to_string(num_operations["reduce"]);
                if (download_object({data.data() + i * buffer_length, buffer_length}, file_name)) {
                    received[i] = true;
                }
            }
            // Apply function where possible
            bool all_left_applied = true;
            for (int i = 0; i < num_peers; i++) {
                if (received[i] && !applied[i] && (!left_to_right || all_left_applied)) {
                    f.f(recvbuf.buf, data.data() + i * buffer_length);
                    applied[i] = true;
                } else if (!received[i]) {
                    all_left_applied = false;
                }
            }

            elapsed_time += timeout;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
        num_operations["reduce"]++;
        if (std::any_of(applied.begin(), applied.end(), [] (bool v) { return !v; })) {
            throw Utils::Timeout();
        }
    } else {
        std::string file_name = comm_name + std::to_string(peer_id) + "_reduce_" + std::to_string(num_operations["reduce"]);
        num_operations["reduce"]++;
        upload(sendbuf, file_name);
    }
}

void FMI::Comm::ClientServer::scan(channel_data sendbuf, channel_data recvbuf, raw_function f) {
    if (peer_id != num_peers - 1) {
        std::string file_name = comm_name + std::to_string(peer_id) + "_scan_" + std::to_string(num_operations["scan"]);
        upload(sendbuf, file_name);
    }
    bool left_to_right = !(f.commutative && f.associative);
    auto num_data = peer_id + 1;
    std::vector<bool> received(num_data, false);
    std::vector<bool> applied(num_data, false);
    auto buffer_length = sendbuf.len;
    std::vector<char> data(buffer_length * num_data);
    std::memcpy(reinterpret_cast<void*>(recvbuf.buf), sendbuf.buf, buffer_length);
    received[peer_id] = true;
    applied[peer_id] = true;
    unsigned int elapsed_time = 0;
    while (elapsed_time < max_timeout && std::any_of(applied.begin(), applied.end(), [] (bool v) { return !v; }) ) {
        // Receive all values
        for (int i = 0; i < num_data; i++) {
            if (received[i]) {
                continue;
            }
            std::string file_name = comm_name + std::to_string(i) + "_scan_" + std::to_string(num_operations["scan"]);
            if (download_object({data.data() + i * buffer_length, buffer_length}, file_name)) {
                received[i] = true;
            }
        }
        // Apply function where possible
        bool all_left_applied = true;
        for (int i = 0; i < num_peers; i++) {
            if (received[i] && !applied[i] && (!left_to_right || all_left_applied)) {
                f.f(recvbuf.buf, data.data() + i * buffer_length);
                applied[i] = true;
            } else if (!received[i]) {
                all_left_applied = false;
            }
        }

        elapsed_time += timeout;
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    }
    if (std::any_of(applied.begin(), applied.end(), [] (bool v) { return !v; })) {
        throw Utils::Timeout();
    }
    num_operations["scan"]++;
}

FMI::Comm::ClientServer::ClientServer(std::map<std::string, std::string> params) {
    timeout = std::stoi(params["timeout"]);
    max_timeout = std::stoi(params["max_timeout"]);
}

double FMI::Comm::ClientServer::get_operation_latency(FMI::Utils::OperationInfo op_info) {
    std::size_t size_in_bytes = op_info.data_size;
    switch (op_info.op) {
        case Utils::send:
            return get_latency(1, 1, size_in_bytes);
        case Utils::bcast:
            return get_latency(1, num_peers - 1, size_in_bytes);
        case Utils::barrier:
        {
            double upload = get_latency(num_peers, 1, 1);
            double download = get_latency(1, num_peers, 1); // LIST used, modeled as download from one
            return upload + download;
        }
        case Utils::gather:
            return get_latency(num_peers - 1, 1, size_in_bytes);
        case Utils::scatter:
            return get_latency(1, num_peers - 1, size_in_bytes);
        case Utils::reduce:
            return get_latency(num_peers - 1, 1, size_in_bytes);
        case Utils::allreduce:
        {
            double reduction = get_latency(num_peers - 1, 1, size_in_bytes);
            double bcast = get_latency(1, num_peers - 1, size_in_bytes);
            return reduction + bcast;
        }
        case Utils::scan:
            // Pattern is parallel (num_peers - 1, 1), (num_peers - 2, 1), ... -> Slowest one is (num_peers - 1, 1)
            return get_latency(num_peers - 1, 1, size_in_bytes);
    }
    throw std::runtime_error("Operation not implemented");
}

double FMI::Comm::ClientServer::get_operation_price(FMI::Utils::OperationInfo op_info) {
    std::size_t size_in_bytes = op_info.data_size;
    switch (op_info.op) {
        case Utils::send:
            return get_price(1, 1, size_in_bytes);
        case Utils::bcast:
            return get_price(1, num_peers - 1, size_in_bytes);
        case Utils::barrier:
        {
            double upload = get_price(num_peers, 1, 1);
            double download = get_price(1, num_peers, 1);
            return upload + download;
        }
        case Utils::gather:
            return get_price(num_peers - 1, 1, size_in_bytes);
        case Utils::scatter:
            return get_price(1, num_peers - 1, size_in_bytes);
        case Utils::reduce:
            return get_price(num_peers - 1, 1, size_in_bytes);
        case Utils::allreduce:
        {
            double reduction = get_price(num_peers - 1, 1, size_in_bytes);
            double bcast = get_price(1, num_peers - 1, size_in_bytes);
            return reduction + bcast;
        }
        case Utils::scan:
            double costs = 0.;
            // N - 1 uploads with varying number of consumers
            for (int i = 1; i < num_peers; i++) {
                costs += get_latency(1, num_peers - i, size_in_bytes);
            }
            return costs;
    }
    throw std::runtime_error("Operation not implemented");
}
