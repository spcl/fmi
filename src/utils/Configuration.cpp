#include "../../include/utils/Configuration.h"
#include <boost/property_tree/json_parser.hpp>
#include <iostream>

namespace SMI::Utils {

    Configuration::Configuration(const std::string& config_path) {
        boost::property_tree::read_json(config_path, root);
        //int test = root.get<int>("test");
    }

    std::map<std::string, std::map<std::string, std::string>> Configuration::get_backends() {
        std::map<std::string, std::map<std::string, std::string>> backends;
        auto backends_tree = root.get_child("backends");
        for (const auto& kv : backends_tree) {
            std::string backend_name = kv.first;
            std::map<std::string, std::string> backend_params;
            for (const auto& backend_tree : kv.second) {
                std::string param_name = backend_tree.first;
                std::string param_val = backend_tree.second.data();
                backend_params[param_name] = param_val;
            }
            backends[backend_name] = backend_params;
        }
        return backends;
    }
}

