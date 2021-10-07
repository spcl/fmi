#include "../../include/comm/PeerToPeer.h"
#include <cmath>
#include <iostream>
#include <cstring>

void SMI::Comm::PeerToPeer::send(channel_data buf, SMI::Utils::peer_num dest) {
    send_object(buf, dest);
}

void SMI::Comm::PeerToPeer::recv(channel_data buf, SMI::Utils::peer_num src) {
    recv_object(buf, src);
}

void SMI::Comm::PeerToPeer::bcast(channel_data buf, SMI::Utils::peer_num root) {
    int rounds = ceil(log2(num_peers));
    Utils::peer_num trans_peer_id = transform_peer_id(peer_id, root, true);
    for (int i = rounds - 1; i >= 0; i--) {
        Utils::peer_num rcpt = trans_peer_id + (Utils::peer_num) std::pow(2, i);
        if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && rcpt < num_peers) {
            Utils::peer_num real_rcpt = transform_peer_id(rcpt, root, false);
            send(buf, real_rcpt);
        } else if (trans_peer_id % (int) std::pow(2, i) == 0 && trans_peer_id % (int) std::pow(2, i + 1) != 0){
            Utils::peer_num real_src = transform_peer_id(trans_peer_id - (int) std::pow(2, i), root, false);
            recv(buf, real_src);
        }
    }
}

void SMI::Comm::PeerToPeer::barrier() {
    char send = 1;
    if (peer_id == 0) {
        char* recv_buffer = new char[num_peers];
        gather({&send, sizeof(char)}, {recv_buffer, num_peers * sizeof(char)}, 0);
    } else {
        gather({&send, sizeof(char)}, {}, 0);
    }
    bcast({&send, sizeof(char)}, 0);
}

void SMI::Comm::PeerToPeer::reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) {

}

void SMI::Comm::PeerToPeer::scan(channel_data sendbuf, channel_data recvbuf, raw_function f) {

}

void SMI::Comm::PeerToPeer::gather(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) {
    int rounds = ceil(log2(num_peers));
    Utils::peer_num trans_peer_id = transform_peer_id(peer_id, root, true);
    std::size_t single_buffer_size = sendbuf.len;
    // Find needed buffer size and allocate it
    if (peer_id != root) {
        unsigned int peers_in_buffer = 1;
        for (int i = rounds - 1; i >= 0; i--) {
            Utils::peer_num src = trans_peer_id + (Utils::peer_num) std::pow(2, i);
            if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && src < num_peers) {
                peers_in_buffer += std::min((Utils::peer_num) std::pow(2, i), num_peers - src);
            }
        }
        recvbuf.buf = new char[peers_in_buffer * single_buffer_size];
        recvbuf.len = peers_in_buffer * single_buffer_size;
        std::memcpy(recvbuf.buf, sendbuf.buf, single_buffer_size);
    } else {
        std::memcpy(recvbuf.buf + single_buffer_size * root, sendbuf.buf, single_buffer_size);
    }

    for (int i = 0; i < rounds; i++) {
        Utils::peer_num src = trans_peer_id + (Utils::peer_num) std::pow(2, i);

        if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && src < num_peers) {
            unsigned int responsible_peers = std::min((Utils::peer_num) std::pow(2, i), num_peers - src);
            std::size_t buf_len = responsible_peers * single_buffer_size;
            Utils::peer_num real_src = transform_peer_id(src, root, false);

            if (peer_id == root) {
                if (real_src * single_buffer_size + buf_len > recvbuf.len) {
                    // Need to wraparound with temporary buffer
                    char *tmp = new char[buf_len];
                    recv({tmp, buf_len}, real_src);
                    unsigned int length_end = recvbuf.len - real_src * single_buffer_size; // How many bytes to copy at end of buffer
                    std::memcpy(recvbuf.buf + real_src * single_buffer_size, tmp, length_end);
                    std::memcpy(recvbuf.buf, tmp + length_end, buf_len - length_end);
                    delete[] tmp;
                } else {
                    recv({recvbuf.buf + real_src * single_buffer_size, buf_len}, real_src);
                }
            } else {
                recv({recvbuf.buf + (src - trans_peer_id) * single_buffer_size, buf_len}, real_src);
            }
        } else if (trans_peer_id % (int) std::pow(2, i) == 0 && trans_peer_id % (int) std::pow(2, i + 1) != 0){
            unsigned int responsible_peers = std::min((Utils::peer_num) std::pow(2, i), num_peers - trans_peer_id);
            std::size_t buf_len = responsible_peers * single_buffer_size;
            Utils::peer_num real_dst = transform_peer_id(trans_peer_id - (int) std::pow(2, i), root, false);
            send({recvbuf.buf, buf_len}, real_dst);
        }
    }
    if (peer_id != root) {
        delete[] recvbuf.buf;
    }
}

void SMI::Comm::PeerToPeer::scatter(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root) {
    int rounds = ceil(log2(num_peers));
    Utils::peer_num trans_peer_id = transform_peer_id(peer_id, root, true);
    std::size_t single_buffer_size = recvbuf.len;
    for (int i = rounds - 1; i >= 0; i--) {
        Utils::peer_num rcpt = trans_peer_id + (Utils::peer_num) std::pow(2, i);


        if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && rcpt < num_peers) {
            unsigned int responsible_peers = std::min((Utils::peer_num) std::pow(2, i), num_peers - rcpt);
            std::size_t buf_len = responsible_peers * single_buffer_size;
            Utils::peer_num real_rcpt = transform_peer_id(rcpt, root, false);

            if (peer_id == root) {
                if (real_rcpt * single_buffer_size + buf_len > sendbuf.len) {
                    // Wrapping around, need to allocate a temporary buffer
                    char* tmp = new char[buf_len];
                    unsigned int length_end = sendbuf.len - real_rcpt * single_buffer_size; // How many bytes we need to send at end of buffer
                    std::memcpy(tmp, sendbuf.buf + real_rcpt * single_buffer_size, length_end);
                    // Copy rest from beginning
                    std::memcpy(tmp + length_end, sendbuf.buf, buf_len - length_end);
                    send({tmp, buf_len}, real_rcpt);
                    delete[] tmp;
                } else {
                    send({sendbuf.buf + real_rcpt * single_buffer_size, buf_len}, real_rcpt);
                }
            } else {
                send({sendbuf.buf + (rcpt - trans_peer_id) * single_buffer_size, buf_len}, real_rcpt);
            }
        } else if (trans_peer_id % (int) std::pow(2, i) == 0 && trans_peer_id % (int) std::pow(2, i + 1) != 0){
            unsigned int responsible_peers = std::min((Utils::peer_num) std::pow(2, i), num_peers - trans_peer_id);
            std::size_t buf_len = responsible_peers * single_buffer_size;
            Utils::peer_num real_src = transform_peer_id(trans_peer_id - (int) std::pow(2, i), root, false);
            sendbuf.buf = new char[buf_len];
            sendbuf.len = buf_len;
            recv(sendbuf, real_src);
        }
    }
    if (peer_id == root) {
        std::memcpy(recvbuf.buf, sendbuf.buf + peer_id * single_buffer_size, single_buffer_size);
    } else {
        std::memcpy(recvbuf.buf, sendbuf.buf, single_buffer_size);
        delete[] sendbuf.buf;
    }
}

SMI::Utils::peer_num SMI::Comm::PeerToPeer::transform_peer_id(SMI::Utils::peer_num id, SMI::Utils::peer_num root, bool forward) {
    if (forward) {
        return (id + num_peers - root) % num_peers; // Transform s.t. root has id 0
    } else {
        return (id + root) % num_peers;
    }
}

