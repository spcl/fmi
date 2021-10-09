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

        std::map< std::string, std::pair< std::map<std::string, std::string>, std::map<std::string, std::string> > > get_active_channels();

        double get_faas_price();

    private:
        boost::property_tree::ptree root;
    };
}



#endif //SMI_CONFIGURATION_H
