#ifndef SMI_CONFIGURATION_H
#define SMI_CONFIGURATION_H


#include <string>
namespace SMI::utils {
    class Configuration {
    public:
        explicit Configuration(const std::string& config_path);
    };
}



#endif //SMI_CONFIGURATION_H
