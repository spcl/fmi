#include "../../include/comm/Direct.h"
#include <tcpunch.h>
#include <sys/socket.h>
#include <boost/log/trivial.hpp>
#include <thread>

SMI::Comm::Direct::Direct(std::map<std::string, std::string> params) {
    hostname = params["host"];
    port = std::stoi(params["port"]);
}

void SMI::Comm::Direct::send_object(channel_data buf, Utils::peer_num rcpt_id) {
    check_socket(rcpt_id, comm_name + std::to_string(peer_id) + "_" + std::to_string(rcpt_id));
    long sent = ::send(sockets[rcpt_id], buf.buf, buf.len, 0);
    if (sent == -1) {
        BOOST_LOG_TRIVIAL(error) << peer_id << ": Error when sending: " << strerror(errno) ;
    }
}

void SMI::Comm::Direct::recv_object(channel_data buf, Utils::peer_num sender_id) {
    check_socket(sender_id, comm_name + std::to_string(sender_id) + "_" + std::to_string(peer_id));
    long received = ::recv(sockets[sender_id], buf.buf, buf.len, MSG_WAITALL);
    if (received == -1 || received < buf.len) {
        BOOST_LOG_TRIVIAL(error) << peer_id << ": Error when receiving: " << strerror(errno);
    }
}

void SMI::Comm::Direct::check_socket(SMI::Utils::peer_num partner_id, std::string pair_name) {
    if (sockets.empty()) {
        sockets = std::vector<int>(num_peers, -1);
    }
    if (sockets[partner_id] == -1) {
        sockets[partner_id] = pair(pair_name, hostname, port);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(sockets[partner_id], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }
}

