#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <nlohmann/json.hpp> // JSONライブラリ
#include "my_config.hpp"

using namespace std;
// JSON用エイリアス
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  データ構造
// ─────────────────────────────────────────────


class DbSearch {
private:
    sqlite3* db_;
    std::string name = "";

    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::cerr << "SQL エラー: " << err << "\n";
            sqlite3_free(err);
        }
    }

    sqlite3_stmt* prepare(const char* sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Prepare 失敗: " << sqlite3_errmsg(db_) << "\n";
            std::exit(1);
        }
        return stmt;
    }

    static std::string col_text(sqlite3_stmt* s, int col) {
        const unsigned char* t = sqlite3_column_text(s, col);
        return t ? reinterpret_cast<const char*>(t) : "";
    }

public:
    explicit DbSearch(const std::string& path) : db_(nullptr) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            std::cerr << "DB オープン失敗: " << sqlite3_errmsg(db_) << "\n";
            std::exit(1);
        }
        exec("PRAGMA journal_mode=WAL;");
        //createTable();
    }
    ~DbSearch() { if (db_) sqlite3_close(db_); }

    int get_count() {
        int ret = 0;
        std::string sql =
            "SELECT COUNT(*) FROM document;";

        sqlite3_stmt* stmt = prepare(sql.c_str());
        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string value = col_text(stmt, 0);
            //cout << "get_count=" << value << "\n";
            int i = std::stoi(value);
            count = i;
        }
        sqlite3_finalize(stmt);
        ret = count;
        return ret;
    }
    
    std::vector<EmbedData> get_items() {
        std::string sql =
            "SELECT id, content, embeddings FROM document;";

        sqlite3_stmt* stmt = prepare(sql.c_str());
        int count = 0;
        std::vector<EmbedData> items;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            EmbedData data;
            data.id      = col_text(stmt, 0);
            data.content      = col_text(stmt, 1);
            std::string emb_str = col_text(stmt, 2);
            json j1 = json::parse(emb_str);
            auto vec = j1;
            int vlength = sizeof(vec) / sizeof(vec[0]);
            //std::cout << "vlen=" << vec.size() << std::endl;
            data.embedding    = vec.get<std::vector<float>>();
            items.push_back(data);
        }
        sqlite3_finalize(stmt);
        return items;
    }

    bool search_embed_size(std::vector<float> embedding) {
        bool ret = false;
        MyConfig config("");
        if (embedding.size() != config.EMBED_SIZE) {
            std::cout << "input size: " << embedding.size() << "\n";
            std::cout << "config.EMBED_SIZE: " << config.EMBED_SIZE << "\n";
            std::cerr << "error , Embedding size mismatch : " << config.EMBED_SIZE << std::endl; 
            return ret;
        }
        ret = true;
        return ret;
    }

    std::string vecSearchJson(std::vector<float> embedding) {
        std::string ret = "";
        try {
            int count =  get_count();
            //cout << "count=" << count << "\n";
            if(count == 0){
                cout << "error, documet none"  << "\n";
                return ret;
            }

            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            //std::cout << "embedding.vlen=" << embedding.size() << std::endl;            

            std::vector<EmbedData> items = get_items();
            std::vector<ResultEmbed> result_items;
            for (const auto& data : items) {
                std::string id = data.id;
                std::vector<float> vec = data.embedding;
                int vlength = sizeof(vec) / sizeof(vec[0]);

                float distance = cosine_similarity(embedding, vec);
                //std::cout << "distance=" << distance << std::endl;            
                ResultEmbed res_item;
                res_item.id = id;
                res_item.embedding = vec;
                res_item.content = data.content;
                res_item.distance = distance;
                //std::cout << data.id << ", distance=" << distance << std::endl;
                if(distance > 0.6) {
                    result_items.push_back(res_item);
                }
            }
            std::sort(result_items.begin(), result_items.end(),
                [](const ResultEmbed& a, const ResultEmbed& b) {
                    return a.distance > b.distance;
                }
            );   

            // 結果表示
            std::string out_str = "";
            for (const auto& item : result_items) {
                //std::cout << "id=" << item.id
                //        << ", content=" << item.content
                //        << ", distance=" << item.distance << std::endl;
                if (out_str.empty()) {
                    std::cout << "distance=" << item.distance
                        << ", id=" << item.id << std::endl;
                    out_str = item.content + "\n";
                }        
            }
            //std::cout << "out_str=" << out_str << std::endl;
            //std::cout << out_str << std::endl;
            return out_str;
        } catch (const exception &e) {
            cerr << e.what() << endl;
            return ret;
        }
        return ret;
    }


    std::string rag_search(std::vector<float> embedding) {
        std::string ret = "";
        try {
            int count =  get_count();
            //cout << "count=" << count << "\n";
            if(count == 0){
                cout << "error, documet none"  << "\n";
                return ret;
            }

            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            //std::cout << "embedding.vlen=" << embedding.size() << std::endl;            

            std::vector<EmbedData> items = get_items();
            std::vector<ResultEmbed> result_items;
            for (const auto& data : items) {
                std::string id = data.id;
                std::vector<float> vec = data.embedding;
                int vlength = sizeof(vec) / sizeof(vec[0]);

                float distance = cosine_similarity(embedding, vec);
                //std::cout << "distance=" << distance << std::endl;            
                ResultEmbed res_item;
                res_item.id = id;
                res_item.embedding = vec;
                res_item.content = data.content;
                res_item.distance = distance;
                //std::cout << data.id << ", distance=" << distance << std::endl;
                if(distance > 0.6) {
                    result_items.push_back(res_item);
                }
            }
            std::sort(result_items.begin(), result_items.end(),
                [](const ResultEmbed& a, const ResultEmbed& b) {
                    return a.distance > b.distance;
                }
            );   

            // 結果表示
            std::string out_str = "";
            for (const auto& item : result_items) {
                //std::cout << "id=" << item.id
                //        << ", content=" << item.content
                //        << ", distance=" << item.distance << std::endl;
                if (out_str.empty()) {
                    std::cout << "distance=" << item.distance
                        << ", id=" << item.id << std::endl;
                    out_str = item.content + "\n";
                }        
            }
            //std::cout << "out_str=" << out_str << std::endl;
            //std::cout << out_str << std::endl;
            return out_str;
        } catch (const exception &e) {
            cerr << e.what() << endl;
            return ret;
        }
        return ret;
    }

    float cosine_similarity(const std::vector<float>& v1, const std::vector<float>& v2) {
        if (v1.size() != v2.size()) {
            throw std::invalid_argument("Vectors must be of the same length.");
        }

        float dot = 0.0f, norm1Sq = 0.0f, norm2Sq = 0.0f;

        for (size_t i = 0; i < v1.size(); i++) {
            const float a = v1[i];
            const float b = v2[i];
            dot     += a * b;
            norm1Sq += a * a;
            norm2Sq += b * b;
        }

        return dot / (std::sqrt(norm1Sq) * std::sqrt(norm2Sq));
    }

};
