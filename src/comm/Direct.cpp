#include "../../include/comm/Direct.h"
#include <tcpunch.h>
#include <sys/socket.h>
#include <boost/log/trivial.hpp>

SMI::Comm::Direct::Direct(std::map<std::string, std::string> params) {
    hostname = params["host"];
    port = std::stoi(params["port"]);
}

void SMI::Comm::Direct::send_object(channel_data buf, Utils::peer_num rcpt_id) {
    if (sockets.empty()) {
        sockets = std::vector<int>(num_peers, -1);
    }
    if (sockets[rcpt_id] == -1) {
        sockets[rcpt_id] = pair(comm_name + std::to_string(peer_id) + "_" + std::to_string(rcpt_id), hostname, port);
    }
    long sent = ::send(sockets[rcpt_id], buf.buf, buf.len, 0);
    if (sent == -1) {
        BOOST_LOG_TRIVIAL(error) << "Error when sending: " << strerror(errno) ;
    }
}

void SMI::Comm::Direct::recv_object(channel_data buf, Utils::peer_num sender_id) {
    if (sockets.empty()) {
        sockets = std::vector<int>(num_peers, -1);
    }
    if (sockets[sender_id] == -1) {
        sockets[sender_id] = pair(comm_name + std::to_string(sender_id) + "_" + std::to_string(peer_id), hostname, port);
    }
    long received = ::recv(sockets[sender_id], buf.buf, buf.len, MSG_WAITALL);
    if (received == -1 || received < buf.len) {
        BOOST_LOG_TRIVIAL(error) << "Error when receiving: " << strerror(errno);
    }
}


