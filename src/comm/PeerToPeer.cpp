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
    Utils::peer_num trans_peer_id = transform_peer_id(peer_id, root, true);
    bool has_received = false; // for uneven number of peers
    for (int i = rounds - 1; i >= 0; i--) {
        Utils::peer_num rcpt = trans_peer_id + (Utils::peer_num) std::pow(2, i);
        if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && rcpt < num_peers) {
            Utils::peer_num real_rcpt = transform_peer_id(rcpt, root, false);
            send(buf, real_rcpt);
        } else if (trans_peer_id % (int) std::pow(2, i) == 0 && !has_received){
            Utils::peer_num real_src = transform_peer_id(trans_peer_id - (int) std::pow(2, i), root, false);
            recv(buf, real_src);
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

SMI::Utils::peer_num SMI::Comm::PeerToPeer::transform_peer_id(SMI::Utils::peer_num id, SMI::Utils::peer_num root, bool forward) {
    if (forward) {
        return (id + num_peers - root) % num_peers; // Transform s.t. root has id 0
    } else {
        return (id + root) % num_peers;
    }
}
