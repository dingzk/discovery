#include "storage/storage.h"
#include "vintage/configservice.h"
#include "vintage/namingservice.h"

#include "serializer/rapidjson/document.h"
#include "serializer/rapidjson/writer.h"
#include "serializer/rapidjson/stringbuffer.h"
#include <iostream>
using namespace rapidjson;


#include <stdio.h>
#include <string.h>



int json_test()
{
    // 1. 把 JSON 解析至 DOM。
    const char* json =  "{\"service\":\"for-test\",\"cluster\":\"test.for/service\",\"sign\":\"3bb444b2aae425d4e31dc04f717134fd\",\"nodes\":{\"working\":[{\"host\":\"127.0.0.1:80\",\"extInfo\":\"\"},{\"host\":\"172.16.139.72:80\",\"extInfo\":\"\"},{\"host\":\"172.16.139.71:80\",\"extInfo\":\"{\\\"ancd\\\":\\\"cc\\\",\\\"weight\\\":122,\\\"falcon_tag\\\":\\\"static\\\"}\"}],\"unreachable\":[]}}";
    Document d;
    d.Parse(json);

    // 2. 利用 DOM 作出修改。
//    Value& s = d["stars"];
//    s.SetInt(s.GetInt() + 1);

    assert(d.IsObject());
    assert(d.HasMember("sign"));
    rapidjson::Value& s = d["sign"];
    std::cout << "lookup: sign : " << d["sign"].GetString() << std::endl;


    // 3. 把 DOM 转换（stringify）成 JSON。
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
    return 0;

}

int main(int argc, char **argv)
{
    json_test();
    return 0;
}