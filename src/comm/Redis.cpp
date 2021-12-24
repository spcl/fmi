#include "../../include/comm/Redis.h"
#include <boost/log/trivial.hpp>

FMI::Comm::Redis::Redis(std::map<std::string, std::string> params, std::map<std::string, std::string> model_params) : ClientServer(params) {
    std::string hostname = params["host"];
    auto port = std::stoi(params["port"]);
    bandwidth_single = std::stod(model_params["bandwidth_single"]);
    bandwidth_multiple = std::stod(model_params["bandwidth_multiple"]);
    overhead = std::stod(model_params["overhead"]);
    transfer_price = std::stod(model_params["transfer_price"]);
    instance_price = std::stod(model_params["instance_price"]);
    requests_per_hour = std::stoi(model_params["requests_per_hour"]);
    if (model_params["include_infrastructure_costs"] == "true") {
        include_infrastructure_costs = true;
    } else {
        include_infrastructure_costs = false;
    }

    context = redisConnect(hostname.c_str(), port);
    if (context == nullptr || context->err) {
        if (context) {
            BOOST_LOG_TRIVIAL(error) << "Error when connecting to Redis: " << context->errstr;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Allocating Redis context not possible";
        }
    }
}

FMI::Comm::Redis::~Redis() {
    redisFree(context);
}

void FMI::Comm::Redis::upload_object(channel_data buf, std::string name) {
    std::string command = "SET " + name + " %b";
    auto* reply = (redisReply*) redisCommand(context, command.c_str(), buf.buf, buf.len);
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "Error when uploading to Redis: " << reply->str;
    }
    freeReplyObject(reply);
}

bool FMI::Comm::Redis::download_object(channel_data buf, std::string name) {
    std::string command = "GET " + name;
    auto* reply = (redisReply*) redisCommand(context, command.c_str());
    if (reply->type == REDIS_REPLY_NIL || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    } else {
        std::memcpy(buf.buf, reply->str, std::min(buf.len, reply->len));
        freeReplyObject(reply);
        return true;
    }
}

void FMI::Comm::Redis::delete_object(std::string name) {
    std::string command = "DEL " + name;
    auto* reply = (redisReply*) redisCommand(context, command.c_str());
    freeReplyObject(reply);
}

std::vector<std::string> FMI::Comm::Redis::get_object_names() {
    std::vector<std::string> keys;
    std::string command = "KEYS *";
    auto* reply = (redisReply*) redisCommand(context, command.c_str());
    for (int i = 0; i < reply->elements; i++) {
        keys.emplace_back(reply->element[i]->str);
    }
    return keys;
}

double FMI::Comm::Redis::get_latency(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double agg_bandwidth = std::min(producer * consumer * bandwidth_single, bandwidth_multiple);
    double trans_time = producer * consumer * ((double) size_in_bytes / 1000000.) / agg_bandwidth;
    return std::log2(producer + consumer) * overhead + trans_time;
}

double FMI::Comm::Redis::get_price(Utils::peer_num producer, Utils::peer_num consumer, std::size_t size_in_bytes) {
    double transfer_costs = (1 + consumer) * producer * ((double) size_in_bytes / 1000000000.) * transfer_price;
    double total_costs = transfer_costs;
    if (include_infrastructure_costs) {
        total_costs += 1. / requests_per_hour * instance_price;
    }
    return total_costs;
}

