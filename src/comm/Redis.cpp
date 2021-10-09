#include "../../include/comm/Redis.h"
#include <boost/log/trivial.hpp>

SMI::Comm::Redis::Redis(std::map<std::string, std::string> params, std::map<std::string, std::string> perf_params) : ClientServer(params) {
    std::string hostname = params["host"];
    auto port = std::stoi(params["port"]);
    context = redisConnect(hostname.c_str(), port);
    if (context == nullptr || context->err) {
        if (context) {
            BOOST_LOG_TRIVIAL(error) << "Error when connecting to Redis: " << context->errstr;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Allocating Redis context not possible";
        }
    }
}

SMI::Comm::Redis::~Redis() {
    redisFree(context);
}

void SMI::Comm::Redis::upload_object(channel_data buf, std::string name) {
    std::string command = "SET " + name + " %b";
    auto* reply = (redisReply*) redisCommand(context, command.c_str(), buf.buf, buf.len);
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "Error when uploading to Redis: " << reply->str;
    }
    freeReplyObject(reply);
}

bool SMI::Comm::Redis::download_object(channel_data buf, std::string name) {
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

void SMI::Comm::Redis::delete_object(std::string name) {
    std::string command = "DEL " + name;
    auto* reply = (redisReply*) redisCommand(context, command.c_str());
    freeReplyObject(reply);
}

std::vector<std::string> SMI::Comm::Redis::get_object_names() {
    std::vector<std::string> keys;
    std::string command = "KEYS *";
    auto* reply = (redisReply*) redisCommand(context, command.c_str());
    for (int i = 0; i < reply->elements; i++) {
        keys.emplace_back(reply->element[i]->str);
    }
    return keys;
}

double SMI::Comm::Redis::get_bandwidth(SMI::Utils::peer_num producers, SMI::Utils::peer_num consumers) {
    return 0.0;
}

double SMI::Comm::Redis::get_overhead() {
    return 0.0;
}
