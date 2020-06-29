//
// Created by zhenkai on 2020/6/29.
// 基于rapidjson的封装
//

#ifndef DISCOVERY_JSON_H
#define DISCOVERY_JSON_H

#include "serializer/rapidjson/document.h"
#include "serializer/rapidjson/writer.h"
#include "serializer/rapidjson/stringbuffer.h"
#include <string>

class Json
{
public:
    static bool encode(const rapidjson::Value &v, std::string &str);
    static bool decode(std::string &str, rapidjson::Document &root);
    static bool decode(const char *str, rapidjson::Document &root);
    static bool decode(const char *str, size_t len, rapidjson::Document &root);
private:
    // Prohibit copy constructor & assignment operator.
    Json(const Json&);
    Json& operator=(const Json&);
};


#endif //DISCOVERY_JSON_H
