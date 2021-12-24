#ifndef SMI_DIRECT_H
#define SMI_DIRECT_H

#include "PeerToPeer.h"

namespace FMI::Comm {
    //! Channel that uses the TCPunch TCP NAT Hole Punching Library for connection establishment.
    class Direct : public PeerToPeer {
    public:
        explicit Direct(std::map<std::string, std::string> params, std::map<std::string, std::string> model_params);

        void send_object(channel_data buf, Utils::peer_num rcpt_id) override;

        void recv_object(channel_data buf, Utils::peer_num sender_id) override;

        double get_latency(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) override;

        double get_price(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) override;

    private:
        //! Contains the socket file descriptor for the communication with the peers.
        std::vector<int> sockets;
        std::string hostname;
        int port;
        unsigned int max_timeout;
        // Model params
        double bandwidth;
        double overhead;
        double transfer_price;
        double vm_price;
        unsigned int requests_per_hour;
        bool include_infrastructure_costs;

        //! Checks if connection with a peer partner_id is already established, otherwise establishes it using TCPunch.
        void check_socket(Utils::peer_num partner_id, std::string pair_name);
    };
}



#endif //SMI_DIRECT_H
