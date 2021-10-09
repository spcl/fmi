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
    auto nop = [] (char* a, char* b) {};
    char send = 1;
    allreduce({&send, sizeof(char)}, {&send, sizeof(char)}, {nop, true, true});
}

void SMI::Comm::PeerToPeer::reduce(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, raw_function f) {
    bool left_to_right = !(f.commutative && f.associative);
    if (left_to_right) {
        reduce_ltr(sendbuf, recvbuf, root, f);
    } else {
        reduce_no_order(sendbuf, recvbuf, root, f);
    }
}

void SMI::Comm::PeerToPeer::reduce_ltr(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, const raw_function& f) {
    if (peer_id == root) {
        std::size_t tmpbuf_len = sendbuf.len * num_peers;
        char* tmpbuf = new char[tmpbuf_len];
        gather(sendbuf, {tmpbuf, tmpbuf_len}, root);
        std::memcpy(reinterpret_cast<void*>(recvbuf.buf), tmpbuf, sendbuf.len);
        for (std::size_t i = sendbuf.len; i < tmpbuf_len; i += sendbuf.len) {
            f.f(recvbuf.buf, tmpbuf + i);
        }
        delete[] tmpbuf;
    } else {
        gather(sendbuf, {}, root);
    }
}

void SMI::Comm::PeerToPeer::reduce_no_order(channel_data sendbuf, channel_data recvbuf, SMI::Utils::peer_num root, const raw_function& f) {
    int rounds = ceil(log2(num_peers));
    Utils::peer_num trans_peer_id = transform_peer_id(peer_id, root, true);
    if (peer_id != root) {
        recvbuf.buf = new char[sendbuf.len];
        recvbuf.len = sendbuf.len;
    }
    for (int i = 0; i < rounds; i++) {
        Utils::peer_num src = trans_peer_id + (Utils::peer_num) std::pow(2, i);

        if (trans_peer_id % (int) std::pow(2, i + 1) == 0 && src < num_peers) {
            Utils::peer_num real_src = transform_peer_id(src, root, false);
            recv({recvbuf.buf, recvbuf.len}, real_src);
            f.f(sendbuf.buf, recvbuf.buf);

        } else if (trans_peer_id % (int) std::pow(2, i) == 0 && trans_peer_id % (int) std::pow(2, i + 1) != 0){
            Utils::peer_num real_dst = transform_peer_id(trans_peer_id - (int) std::pow(2, i), root, false);
            send({sendbuf.buf, sendbuf.len}, real_dst);
        }
    }
    if (peer_id == root) {
        std::memcpy(recvbuf.buf, sendbuf.buf, sendbuf.len);
    } else {
        delete[] recvbuf.buf;
    }
}

void SMI::Comm::PeerToPeer::allreduce(channel_data sendbuf, channel_data recvbuf, raw_function f) {
    bool left_to_right = !(f.commutative && f.associative);
    if (left_to_right) {
        reduce(sendbuf, recvbuf, 0, f);
        bcast(recvbuf, 0);
    } else {
        allreduce_no_order(sendbuf, recvbuf, f);
    }
}

void SMI::Comm::PeerToPeer::allreduce_no_order(channel_data sendbuf, channel_data recvbuf, const raw_function &f) {
    // Non power of two N: First receive from processes with ID >= 2^ceil(log2(N)), send result after reduction
    int rounds = floor(log2(num_peers));
    int nearest_power_two = (int) std::pow(2, rounds);
    if (num_peers > nearest_power_two) {
        if (peer_id < nearest_power_two && peer_id + nearest_power_two < num_peers) {
            recv(recvbuf, peer_id + nearest_power_two);
            f.f(sendbuf.buf, recvbuf.buf);
        } else if (peer_id >= nearest_power_two) {
            send(sendbuf, peer_id - nearest_power_two);
        }
    }
    if (peer_id < nearest_power_two) {
        // Actual recursive doubling
        for (int i = 0; i < rounds; i++) {
            int peer = peer_id ^ (int) std::pow(2, i);
            if (peer < peer_id) {
                send(sendbuf, peer);
                recv(recvbuf, peer);
            } else {
                recv(recvbuf, peer);
                send(sendbuf, peer);
            }
            f.f(sendbuf.buf, recvbuf.buf);
        }
    }
    if (num_peers > nearest_power_two) {
        if (peer_id < nearest_power_two && peer_id + nearest_power_two < num_peers) {
            send(sendbuf, peer_id + nearest_power_two);
        } else if (peer_id >= nearest_power_two) {
            recv(sendbuf, peer_id - nearest_power_two);
        }
    }
    std::memcpy(recvbuf.buf, sendbuf.buf, sendbuf.len);
}

void SMI::Comm::PeerToPeer::scan(channel_data sendbuf, channel_data recvbuf, raw_function f) {
    int rounds = floor(log2(num_peers));
    for (int i = 0; i < rounds; i ++) {
        if ((peer_id & ((int) std::pow(2, i + 1) - 1)) == (int) std::pow(2, i + 1) - 1) {
            Utils::peer_num src = peer_id - (int) std::pow(2, i);
            recv(recvbuf, src);
            f.f(sendbuf.buf, recvbuf.buf);
        } else if ((peer_id & ((int) std::pow(2, i) - 1)) == (int) std::pow(2, i) - 1) {
            Utils::peer_num dst = peer_id + (int) std::pow(2, i);
            if (dst < num_peers) {
                send(sendbuf, dst);
                break;
            }
        }
    }
    for (int i = rounds; i > 0; i--) {
        if ((peer_id & ((int) std::pow(2, i) - 1)) == (int) std::pow(2, i) - 1) {
            Utils::peer_num dst = peer_id + (int) std::pow(2, i - 1);
            if (dst < num_peers) {
                send(sendbuf, dst);
            }
        } else if ((peer_id & ((int) std::pow(2, i - 1) - 1)) == (int) std::pow(2, i - 1) - 1) {
            int src = peer_id - (int) std::pow(2, i - 1);
            if (src > 0) {
                recv(recvbuf, src);
                f.f(sendbuf.buf, recvbuf.buf);
            }
        }
    }
    std::memcpy(recvbuf.buf, sendbuf.buf, sendbuf.len);
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

double SMI::Comm::PeerToPeer::get_operation_latency(SMI::Utils::OperationInfo op_info) {
    std::size_t size_in_bytes = op_info.data_size;
    switch (op_info.op) {
        case Utils::send:
            return get_latency(1, 1, size_in_bytes);
        case Utils::bcast:
            return ceil(log2(num_peers)) * get_latency(1, 1, size_in_bytes);
        case Utils::reduce:
            if (!op_info.left_to_right) {
                return ceil(log2(num_peers)) * get_latency(1, 1, size_in_bytes);
            } // else, gather used
        case Utils::gather:
        case Utils::scatter:
        {
            // ceil(log2(num_peers)) rounds, doubling buffer size in each round
            double latency = 0.;
            for (int i = 1; i <= ceil(log2(num_peers)); i++) {
                // Overapproximation for non power of two (too large file sizes)
                latency += get_latency(1, 1, i * size_in_bytes);
            }
            return latency;
        }
        case Utils::barrier:
            size_in_bytes = 1;
        case Utils::allreduce:
            if (op_info.left_to_right) {
                // Reduce with gather, followed by bcast
                double latency = 0.;
                for (int i = 1; i <= ceil(log2(num_peers)); i++) {
                    latency += get_latency(1, 1, i * size_in_bytes);
                }
                latency += ceil(log2(num_peers)) * get_latency(1, 1, size_in_bytes);
                return latency;
            } else {
                // Send and recv in every round
                double latency = 2 * floor(log2(num_peers)) * get_latency(1, 1, size_in_bytes);
                if (floor(log2(num_peers)) != num_peers) {
                    // 2 additional rounds in beginning / end
                    latency += 2 * get_latency(1, 1, size_in_bytes);
                }
                return latency;
            }
        case Utils::scan:
            return 2 * floor(log2(num_peers)) * get_latency(1, 1, size_in_bytes);
    }

}

double SMI::Comm::PeerToPeer::get_operation_price(SMI::Utils::OperationInfo op_info) {
    std::size_t size_in_bytes = op_info.data_size;
    switch (op_info.op) {
        case Utils::send:
            return get_price(1, 1, size_in_bytes);
        case Utils::bcast:
        {
            // Number of comm. doubles in every round, for non-power-of two one additional message per node
            double power_of_two = std::pow(2, floor(log2(num_peers)));
            double comm_rounds = power_of_two - 1 + (num_peers - power_of_two);
            return comm_rounds * get_price(1, 1, size_in_bytes);
        }
        case Utils::reduce:
            if (!op_info.left_to_right) {
                double power_of_two = std::pow(2, floor(log2(num_peers)));
                double comm_rounds = power_of_two - 1 + (num_peers - power_of_two);
                return comm_rounds * get_price(1, 1, size_in_bytes);
            } // else, gather used
        case Utils::gather:
        case Utils::scatter:
        {
            double costs = 0.;
            for (int i = 1; i <= ceil(log2(num_peers)); i++) {
                // Overapproximation for non power of two (too large file sizes, too many comm.)
                costs += std::pow(2, floor(log2(num_peers)) - i) * get_price(1, 1, i * size_in_bytes);
            }
            return costs;
        }
        case Utils::barrier:
        size_in_bytes = 1;
        case Utils::allreduce:
        if (op_info.left_to_right) {
            // Reduce with gather, followed by bcast
            double costs = 0.;
            for (int i = 1; i <= ceil(log2(num_peers)); i++) {
                // Overapproximation for non power of two
                costs += std::pow(2, floor(log2(num_peers)) - i) * get_price(1, 1, i * size_in_bytes);
            }
            double power_of_two = std::pow(2, floor(log2(num_peers)));
            double comm_rounds = power_of_two - 1 + (num_peers - power_of_two);
            costs += comm_rounds * get_price(1, 1, size_in_bytes);
            return costs;
        } else {
            // Send and recv in every round
            double power_of_two = std::pow(2, floor(log2(num_peers)));
            double comm_rounds = 2 * (power_of_two - 1) + 2 * (num_peers - power_of_two);
            return comm_rounds * get_price(1, 1, size_in_bytes);
        }
        case Utils::scan:
            // Bounded by 2 * num_peers: https://link.springer.com/content/pdf/10.1007%2F11846802.pdf
            return 2 * num_peers * get_price(1, 1, size_in_bytes);
    }
}



