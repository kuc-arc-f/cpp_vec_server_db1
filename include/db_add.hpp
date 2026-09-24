#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <uuid/uuid.h>
#include "my_config.hpp"

using namespace std;

class DbAdd {
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

public:
    explicit DbAdd(const std::string& path) : db_(nullptr) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            std::cerr << "DB オープン失敗: " << sqlite3_errmsg(db_) << "\n";
            std::exit(1);
        }
        exec("PRAGMA journal_mode=WAL;");
        //createTable();
    }

    void add_embed(std::vector<float> embedding, std::string content) {
        try {
            uuid_t uuid;
            char uuid_str[37]; // 36文字 + NULL

            // UUID生成（ランダム）
            uuid_generate(uuid);
            // 文字列に変換
            uuid_unparse(uuid, uuid_str);

            std::cout << "UUID: " << uuid_str << std::endl;            
            MyConfig config("");
            //std::cout << "embedding.size=" << embedding.size() << "\n";
            if (embedding.size() != config.EMBED_SIZE) {
                std::cout << "input size: " << embedding.size() << "\n";
                std::cout << "config.EMBED_SIZE: " << config.EMBED_SIZE << "\n";
                std::cerr << "error , Embedding size mismatch" << std::endl; 
                return;
            }

            stringstream ss;
            ss << "[";
            for (size_t i = 0; i < embedding.size(); ++i) {
                if (i > 0) ss << ",";
                ss << embedding[i];
            }
            ss << "]";

            string emb_str = ss.str();
            std::string new_id= uuid_str;

            const char* sql =
                "INSERT INTO document (id, content, embeddings) VALUES (?, ?, ?);";
            sqlite3_stmt* stmt = prepare(sql);
            sqlite3_bind_text(stmt, 1, new_id.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, content.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, emb_str.c_str(), -1, SQLITE_TRANSIENT);
            bool ok = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
            return;            
        } catch (const exception & e) {
            cerr << e.what() << endl;
        }
    }

    ~DbAdd() { if (db_) sqlite3_close(db_); }

};
