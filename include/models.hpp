#pragma once
#include <iostream>

struct QueItem {
    std::string sql;
    std::string uuid;
};
struct EmbedData {
    std::string id;
    std::vector<float> embedding;
    std::string content;
};
struct ResultEmbed {
    std::string id;
    std::vector<float> embedding;
    std::string content;
    float distance;
};
struct SearchDbItem {
    std::string id;
    std::string content;
    float distance;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchDbItem, id, content, distance)

struct ActionReq {
    std::string action_name;
    std::string table;
    std::string sql;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionReq, action_name, table, sql)

struct NormalRespopnse {
    int ret_code;
    std::string message;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NormalRespopnse, ret_code, message)

struct SearchListResp {
    int ret_code;
    std::string data;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchListResp, ret_code, data)

struct LargeJsonRes {
    std::string result;
    std::string text;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LargeJsonRes, result , text)

struct ActionSelectReq {
    std::string action_name;
    std::string table;
    std::string sql;
};
