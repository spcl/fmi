#ifndef SMI_PEERTOPEER_H
#define SMI_PEERTOPEER_H

#include "Channel.h"

namespace FMI::Comm {
    //! Peer-To-Peer channel type
    /*!
     * This class provides optimized collectives for channels where clients can address each other directly and defines the interface that these channels need to implement.
     */
    class PeerToPeer : public Channel {
    public:
        void send(channel_data buf, FMI::Utils::peer_num dest) override;

        void recv(channel_data buf, FMI::Utils::peer_num src) override;

        //! Binomial tree broadcast implementation
        void bcast(channel_data buf, FMI::Utils::peer_num root) override;

        //! Calls allreduce with a (associative and commutative) NOP operation
        void barrier() override;

        //! Binomial tree gather.
        /*!
         * In the beginning, the needed buffer size (largest value that this peer will receive) is determined and a buffer is allocated.
         * If the ID of the root is not 0, we cannot necessarily receive all values directly in recvbuf because we need to wrap around (e.g., when we get from peer N - 1 the values for N - 1, 0, and 1).
         * This is solved by allocating a temporary buffer and copying the values.
         */
        void gather(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root) override;

        //! Binomial tree scatter
        /*!
         * Similarly to gather, the root may need to send values from its sendbuf that is not consecutive when its ID is not 0, which is solved with a temporary buffer.
         */
        void scatter(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root) override;

        //! Calls reduce_no_order for associative and commutative functions, reduce_ltr otherwise
        void reduce(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root, raw_function f) override;

        //! For associative and commutative functions, allreduce_no_order is called. Otherwise, reduce followed by bcast is used.
        void allreduce(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        //! For associative and commutative functions, scan_no_order is called. Otherwise, scan_ltr is called
        void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        //! Send an object to peer with ID peer_id. Needs to be implemented by the channels.
        virtual void send_object(channel_data buf, Utils::peer_num peer_id) = 0;

        //! Receive an object from peer with ID peer_id. Needs to be implemented by the channels.
        virtual void recv_object(channel_data buf, Utils::peer_num peer_id) = 0;

        double get_operation_latency(Utils::OperationInfo op_info) override;

        double get_operation_price(Utils::OperationInfo op_info) override;

    protected:
        //! Reduction with left-to-right evaluation, gather followed by a function evaluation on the root peer.
        void reduce_ltr(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root, const raw_function& f);

        //! Binomial tree reduction where all peers apply the function in every step.
        void reduce_no_order(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root, const raw_function& f);

        //! Recursive doubling allreduce implementation. When num_peers is not a power of two, there is an additional message in the beginning and end for every peer where they send their value / receive the reduced value.
        void allreduce_no_order(channel_data sendbuf, channel_data recvbuf, const raw_function& f);

        //! Linear function application / sending
        void scan_ltr(channel_data sendbuf, channel_data recvbuf, const raw_function& f);

        //! Binomial tree with up- and down-phase
        void scan_no_order(channel_data sendbuf, channel_data recvbuf, const raw_function& f);

    private:
        //! Allows to implement all collectives as if root were 0
        /*!
         * Transforms peer IDs such that the the user-provided root ID has a transformed ID of 0.
         * Makes the implementation of many collectives easier, because they only need to be implemented for the case with root = 0, when the transformation is used in the appropriate places
         * @param id ID to transform
         * @param root User-chosen root ID
         * @param forward Forward (root -> 0) or backward (0 -> root) transformation
         * @return transformed peer ID
         */
        Utils::peer_num transform_peer_id(Utils::peer_num id, Utils::peer_num root, bool forward);

    };
}



#endif //SMI_PEERTOPEER_H
