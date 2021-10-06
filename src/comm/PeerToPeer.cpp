#include "../../include/comm/PeerToPeer.h"

void SMI::Comm::PeerToPeer::send(channel_data buf, SMI::Utils::peer_num dest) {
    send_object(buf, dest);
}

void SMI::Comm::PeerToPeer::recv(channel_data buf, SMI::Utils::peer_num src) {
    recv_object(buf, src);
}

void SMI::Comm::PeerToPeer::bcast(channel_data buf, SMI::Utils::peer_num root) {

}

void SMI::Comm::PeerToPeer::barrier() {

}

void SMI::Comm::PeerToPeer::reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) {

}

void SMI::Comm::PeerToPeer::scan(channel_data sendbuf, channel_data recvbuf, raw_function f) {

}
