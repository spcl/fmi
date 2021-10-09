#include "../../include/utils/Configuration.h"
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <utility>

namespace SMI::Utils {

    Configuration::Configuration(const std::string& config_path) {
        boost::property_tree::read_json(config_path, root);
    }

    std::map< std::string, std::pair< std::map<std::string, std::string>, std::map<std::string, std::string> > > Configuration::get_active_channels() {
        std::map< std::string, std::pair< std::map<std::string, std::string>, std::map<std::string, std::string> > > backends;
        auto backends_tree = root.get_child("backends");
        auto perfdata_tree = root.get_child("performance");
        for (const auto& kv : backends_tree) {
            std::string backend_name = kv.first;
            std::map<std::string, std::string> backend_params;
            for (const auto& backend_tree : kv.second) {
                std::string param_name = backend_tree.first;
                std::string param_val = backend_tree.second.data();
                backend_params[param_name] = param_val;
            }
            if (backend_params.count("enabled") && backend_params["enabled"] != "true") {
                continue;
            }
            std::map<std::string, std::string> perf_params;
            if (perfdata_tree.count(backend_name)) {
                for (const auto& perf_tree : perfdata_tree.get_child(backend_name)) {
                    std::string param_name = perf_tree.first;
                    std::string param_val = perf_tree.second.data();
                    perf_params[param_name] = param_val;
                }
            }
            backends[backend_name] = std::make_pair(backend_params, perf_params);
        }
        return backends;
    }
}

