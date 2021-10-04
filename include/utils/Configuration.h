#ifndef SMI_CONFIGURATION_H
#define SMI_CONFIGURATION_H


#include <string>
#include <map>
#include <any>
#include <boost/property_tree/ptree.hpp>

namespace SMI::Utils {
    class Configuration {
    public:
        explicit Configuration(const std::string& config_path);

        std::map<std::string, std::map<std::string, std::string>> get_active_channels();

    private:
        boost::property_tree::ptree root;
    };
}



#endif //SMI_CONFIGURATION_H
