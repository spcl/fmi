#ifndef FMI_CLIENTSERVER_H
#define FMI_CLIENTSERVER_H

#include "Channel.h"
#include <map>
#include "../utils/Common.h"

namespace FMI::Comm {
    //! Client-Server channel type
    /*!
     * This class provides optimized collectives for client-server channels and defines the interface that these channels need to implement.
     */
    class ClientServer : public Channel {
    public:
        explicit ClientServer(std::map<std::string, std::string> params);

        //! Constructs file / key name based on sender and recipient and then uploads the data.
        void send(channel_data buf, FMI::Utils::peer_num dest) override;

        //! Waits until the object with the expected file / key name appears (or a timeout occurs), then downloads it.
        void recv(channel_data buf, FMI::Utils::peer_num dest) override;

        //! Root uploads its data, all other peers download the object
        void bcast(channel_data buf, FMI::Utils::peer_num root) override;

        //! All peers upload a 1 byte file and wait until num_peers files (associated to this operation based on the file name) exist
        void barrier() override;

        //! All peers upload their data. The root peer downloads these objects and applies the function (as soon as objects become available for associative / commutative functions, left-to-right otherwise)
        void reduce(channel_data sendbuf, channel_data recvbuf, FMI::Utils::peer_num root, raw_function f) override;

        //! All peers upload their data and download the needed files to apply the function. Left-to-right evaluation order is enforced for non-commutative / non-associative functions.
        void scan(channel_data sendbuf, channel_data recvbuf, raw_function f) override;

        //! Function to upload data with a given name / key to the server, needs to be implemented by the channels and should never be invoked directly (use upload instead).
        virtual void upload_object(channel_data buf, std::string name) = 0;

        //! Function to download data with a given name / key from the server, needs to be implemented by the channels. Returns true when download was successful, false when file does not exist.
        virtual bool download_object(channel_data buf, std::string name) = 0;

        //! Try the download (using download_object) until the object appears or the timeout was reached.
        virtual void download(channel_data buf, std::string name);

        //! Uploads objects and keeps track of them.
        virtual void upload(channel_data buf, std::string name);

        //! List all the currently existing objects, needs to be implemented by channels. Needed by some collectives that check for the existence of files, but do not care about their content.
        virtual std::vector<std::string> get_object_names() = 0;

        //! Delete the object with the given name, needs to be implemented by channels.
        virtual void delete_object(std::string name) = 0;

        //! Deletes all objects that were created during the execution.
        void finalize() override;

        double get_operation_latency(Utils::OperationInfo op_info) override;

        double get_operation_price(Utils::OperationInfo op_info) override;

    protected:
        //! Ensures that there are no file / key name conflicts when a collective operation is used multiple times, values are integrated into the file / key name for these operations.
        std::map<std::string, unsigned int> num_operations = {
                {"bcast", 0},
                {"barrier", 0},
                {"reduce", 0},
                {"scan", 0}
        };
        //! Tracks all created objects such that they can be selectively deleted when finalize is called.
        std::vector<std::string> created_objects;
        //! Backoff time when an object does not exist.
        unsigned int timeout;
        //! Maximum time to wait until an object appears, exception is thrown when this value is exceeded.
        unsigned int max_timeout;
    };
}



#endif //FMI_CLIENTSERVER_H
