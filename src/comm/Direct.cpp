#include "../../include/comm/Direct.h"
#include <tcpunch.h>
#include <sys/socket.h>
#include <boost/log/trivial.hpp>
#include <thread>
#include <netinet/tcp.h>

SMI::Comm::Direct::Direct(std::map<std::string, std::string> params, std::map<std::string, std::string> model_params) {
    hostname = params["host"];
    port = std::stoi(params["port"]);
    max_timeout = std::stoi(params["max_timeout"]);
    bandwidth = std::stod(model_params["bandwidth"]);
    overhead = std::stod(model_params["overhead"]);
    transfer_price = std::stod(model_params["transfer_price"]);
    vm_price = std::stod(model_params["vm_price"]);
    requests_per_hour = std::stoi(model_params["requests_per_hour"]);
    if (model_params["include_infrastructure_costs"] == "true") {
        include_infrastructure_costs = true;
    } else {
        include_infrastructure_costs = false;
    }
}

void SMI::Comm::Direct::send_object(channel_data buf, Utils::peer_num rcpt_id) {
    check_socket(rcpt_id, comm_name + std::to_string(peer_id) + "_" + std::to_string(rcpt_id));
    long sent = ::send(sockets[rcpt_id], buf.buf, buf.len, 0);
    if (sent == -1) {
        if (errno == EAGAIN) {
            throw Utils::Timeout();
        }
        BOOST_LOG_TRIVIAL(error) << peer_id << ": Error when sending: " << strerror(errno) ;
    }
}

void SMI::Comm::Direct::recv_object(channel_data buf, Utils::peer_num sender_id) {
    check_socket(sender_id, comm_name + std::to_string(sender_id) + "_" + std::to_string(peer_id));
    long received = ::recv(sockets[sender_id], buf.buf, buf.len, MSG_WAITALL);
    if (received == -1 || received < buf.len) {
        if (errno == EAGAIN) {
            throw Utils::Timeout();
        }
        BOOST_LOG_TRIVIAL(error) << peer_id << ": Error when receiving: " << strerror(errno);
    }
}

void SMI::Comm::Direct::check_socket(SMI::Utils::peer_num partner_id, std::string pair_name) {
    if (sockets.empty()) {
        sockets = std::vector<int>(num_peers, -1);
    }
    if (sockets[partner_id] == -1) {
        try {
            sockets[partner_id] = pair(pair_name, hostname, port, max_timeout);
        } catch (Timeout) {
            throw Utils::Timeout();
        }

        struct timeval timeout;
        timeout.tv_sec = max_timeout / 1000;
        timeout.tv_usec = (max_timeout % 1000) * 1000;
        setsockopt(sockets[partner_id], SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
        setsockopt(sockets[partner_id], SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout);
        // Disable Nagle algorithm to avoid 40ms TCP ack delays
        int one = 1;
        setsockopt(sockets[partner_id], SOL_TCP, TCP_NODELAY, &one, sizeof(one));
    }
}

double SMI::Comm::Direct::get_latency(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double agg_bandwidth = bandwidth;
    double trans_time = producer * consumer * ((double) size_in_bytes / 1000000.) / agg_bandwidth;
    return overhead + trans_time;
}

double SMI::Comm::Direct::get_price(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double transfer_costs = 2 * consumer * producer * ((double) size_in_bytes / 1000000000.) * transfer_price;
    double total_costs = transfer_costs;
    if (include_infrastructure_costs) {
        total_costs += 1. / requests_per_hour * vm_price;
    }
    return total_costs;
}
