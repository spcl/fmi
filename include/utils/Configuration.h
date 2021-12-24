#ifndef FMI_CONFIGURATION_H
#define FMI_CONFIGURATION_H


#include <string>
#include <map>
#include <any>
#include <boost/property_tree/ptree.hpp>

namespace FMI::Utils {
    //! Configuration parser for the FMI JSON configuration file
    class Configuration {
    public:
        explicit Configuration(const std::string& config_path);

        //! Returns the name and a pair with the configuration and model parameters for all channels that are marked active ("active": True) in the config file.
        std::map< std::string, std::pair< std::map<std::string, std::string>, std::map<std::string, std::string> > > get_active_channels();

        //! Returns the configured GiB Second Price for the FaaS platform.
        double get_faas_price();

    private:
        boost::property_tree::ptree root;
    };
}



#endif //FMI_CONFIGURATION_H
