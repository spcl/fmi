#ifndef FMI_COMMON_H
#define FMI_COMMON_H
#include <exception>

//! Contains various utilities that are used in FMI
namespace FMI::Utils {
    //! Type for peer IDs / numbers
    using peer_num = unsigned int;

    //! Custom exception that is thrown on timeouts
    struct Timeout : public std::exception {
        [[nodiscard]] const char * what () const noexcept {
            return "Timeout was reached";
        }
    };

    //! Set by the client, controls the optimization goal of the Channel Policy
    enum Hint {
        fast, cheap
    };

    //! List of currently supported collectives
    enum Operation {
        send, bcast, barrier, gather, scatter, reduce, allreduce, scan
    };

    //! All the information about an operation, passed to the Channel Policy for its decision on which channel to use.
    struct OperationInfo {
        Operation op;
        std::size_t data_size;
        bool left_to_right = false;
    };

}

#endif //FMI_COMMON_H
