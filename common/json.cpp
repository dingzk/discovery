//
// Created by zhenkai on 2020/6/29.
//

#include "json.h"

bool Json::decode(std::string &str, rapidjson::Document &root)
{
    return decode(str.c_str(), root);
}

bool Json::decode(const char *str, rapidjson::Document &root)
{
    if (str == nullptr) {
        return false;
    }
    return !root.Parse(str).HasParseError();
}

bool Json::decode(const char *str, size_t len, rapidjson::Document &root)
{
    if (str == nullptr) {
        return false;
    }
    return !root.Parse(str, len).HasParseError();
}

// need no free
bool Json::encode(const rapidjson::Value &v, std::string &str)
{
    str.clear();
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    str.assign(buffer.GetString());

    return true;
}
