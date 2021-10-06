#include "../../include/comm/PeerToPeer.h"
#include <cmath>
#include <iostream>

void SMI::Comm::PeerToPeer::send(channel_data buf, SMI::Utils::peer_num dest) {
    send_object(buf, dest);
}

void SMI::Comm::PeerToPeer::recv(channel_data buf, SMI::Utils::peer_num src) {
    recv_object(buf, src);
}

void SMI::Comm::PeerToPeer::bcast(channel_data buf, SMI::Utils::peer_num root) {
    int rounds = ceil(log2(num_peers));
    bool has_received = false; // for uneven number of peers
    for (int i = rounds - 1; i >= 0; i--) {
        Utils::peer_num rcpt = peer_id + (Utils::peer_num) std::pow(2, i);
        if (peer_id % (int) std::pow(2, i + 1) == 0 && rcpt < num_peers) {
            send(buf, rcpt);
        } else if (peer_id % (int) std::pow(2, i) == 0 && !has_received){
            recv(buf, peer_id - (int) std::pow(2, i));
            has_received = true;
        }
    }
}

void SMI::Comm::PeerToPeer::barrier() {

}

void SMI::Comm::PeerToPeer::reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) {

}

void SMI::Comm::PeerToPeer::scan(channel_data sendbuf, channel_data recvbuf, raw_function f) {

}
