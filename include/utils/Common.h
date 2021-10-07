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
}

#endif //SMI_COMMON_H
