#include "../../include/utils/Configuration.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>

namespace pt = boost::property_tree;

Configuration::Configuration(const std::string& config_path) {
    pt::ptree root;
    pt::read_json(config_path, root);
    //int test = root.get<int>("test");
}
