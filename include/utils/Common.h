#ifndef SMI_COMMON_H
#define SMI_COMMON_H
#include <exception>

namespace SMI::Utils {
    using peer_num = unsigned int;

    struct Timeout : public std::exception {
        [[nodiscard]] const char * what () const noexcept {
            return "Timeout was reached";
        }
    };

    enum Hint {
        fast, cheap
    };

    enum Operation {
        send, bcast, barrier, gather, scatter, reduce, allreduce, scan
    };

    struct OperationInfo {
        Operation op;
        std::size_t data_size;
        bool left_to_right = false;
    };

}

#endif //SMI_COMMON_H
