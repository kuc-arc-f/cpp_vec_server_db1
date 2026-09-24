#pragma once
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

// JSON用エイリアス
using json = nlohmann::json;

class MemDatabase {
private:
    sqlite3* db;
    
public:
    MemDatabase(const std::string& dbPath = "todos.db") {
        int rc = sqlite3_open(":memory:", &db);
    }
    ~MemDatabase() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool init_import(const char* sql) {
        try{    
            char* errMsg = nullptr;

            if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg)
                != SQLITE_OK) {

                std::cerr << "error: "
                        << errMsg << std::endl;

                sqlite3_free(errMsg);
            }
            std::cout << "init_import , completed." << std::endl;            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "error:" << e.what() << std::endl;
            return false;
        }
    }       
    bool executeSql(const std::string& sql) {
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    sqlite3_stmt* prepare(const char* sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Prepare 失敗: " << sqlite3_errmsg(db) << "\n";
            std::exit(1);
        }
        return stmt;
    }

    static std::string col_text(sqlite3_stmt* s, int col) {
        const unsigned char* t = sqlite3_column_text(s, col);
        return t ? reinterpret_cast<const char*>(t) : "";
    }

    std::string get_uuid(){
        uuid_t uuid;
        char uuid_str[37]; // 36文字 + NULL

        // UUID生成（ランダム）
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);

        std::cout << "UUID: " << uuid_str << std::endl;         
        std::string new_id= uuid_str;
        return new_id;
    }

    // vec_select
    std::string get_sql_delete(
      const std::string& table , const std::string& id
    ) 
    {
      std::string ret = "";
      try{        
        std::string sql = "DELETE FROM " + table;
        sql += " WHERE id = '" + id + "';";

        std::cout << "sql= " << sql << std::endl; 

        return sql;
      } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
      }
      return ret; 
    } 

    std::string get_sql_insert(
      const std::string& table , const std::string& content, std::string embedding
    ) 
    {
      std::string ret = "";
      try{        
        std::string new_id= get_uuid();
        std::cout << "UUID: " << new_id << std::endl;  

        std::string sql = "INSERT INTO " + table;
        sql += "(id, content, embeddings) VALUES ";
        sql += "('" + new_id + "' , '" + content + "', '" + embedding + "'"; 
        sql += ");";

        std::cout << "sql= " << sql << std::endl; 

        return sql;
      } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
      }
      return ret; 
    }    

    bool cache_add(const std::string& id, const std::string& sql_text) {
        sqlite3_stmt* stmt;
        std::string sql = "INSERT INTO system_cache (id, sql) VALUES (?, ?);";
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, sql_text.c_str(), -1, SQLITE_STATIC);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    json selectTableSql(const std::string& tableName, const std::string& sql ) {
        json result;
        result["table"] = tableName;
        result["status"] = "success";

        std::cout << "selectTableSql.sql=" << sql << "\n";            

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            result["status"] = "error";
            result["error"] = sqlite3_errmsg(db);
            return result;
        }

        // カラム情報を取得
        int columnCount = sqlite3_column_count(stmt);
        std::vector<std::string> columnNames;

        for (int i = 0; i < columnCount; i++) {
            columnNames.push_back(sqlite3_column_name(stmt, i));
        }

        // データ行を取得
        json rows = json::array();

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json row;

            for (int i = 0; i < columnCount; i++) {
                const std::string colName = columnNames[i];
                int colType = sqlite3_column_type(stmt, i);

                switch (colType) {
                    case SQLITE_INTEGER:
                        row[colName] = sqlite3_column_int64(stmt, i);
                        break;
                    case SQLITE_FLOAT:
                        row[colName] = sqlite3_column_double(stmt, i);
                        break;
                    case SQLITE_TEXT:
                        row[colName] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                        break;
                    case SQLITE_BLOB:
                        // BLOBデータはBase64などに変換するか、文字列として扱う
                        row[colName] = "[BLOBデータ]";
                        break;
                    case SQLITE_NULL:
                    default:
                        row[colName] = nullptr;
                        break;
                }
            }

            rows.push_back(row);
        }

        sqlite3_finalize(stmt);

        result["columns"] = columnNames;
        result["row_count"] = rows.size();
        result["data"] = rows;

        return result;
    }

    std::vector<QueItem> cache_select_list() {
        std::vector<QueItem> ret;
        std::stringstream json;
        
        const char* sql = "SELECT id , sql from system_cache LIMIT 10000";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return ret;
        }        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            
            const unsigned char* id = sqlite3_column_text(stmt, 0);
            const unsigned char* sql = sqlite3_column_text(stmt, 1);
            QueItem row;
            if(id){
                row.uuid = reinterpret_cast<const char*>(id);
            }else{
                row.uuid = "";
            }
            if(sql){
                row.sql = reinterpret_cast<const char*>(sql);
            }else{
                row.sql = "";
            }
            ret.push_back(row);
        }
        
        sqlite3_finalize(stmt);
        return ret;
    }

    bool cashe_delete(std::string id) {
        std::string sql = "DELETE FROM system_cache WHERE id = ?;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);        
        //sqlite3_bind_int(stmt, 1, id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
        

    // テーブルデータをJSONに変換
    json selectTableToJSON(const std::string& tableName) {
        json result;
        result["table"] = tableName;
        result["status"] = "success";

        std::string sql = "SELECT * FROM " + tableName + ";";
        std::cout << "selectTableToJSON.sql=" << sql << "\n";            

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            result["status"] = "error";
            result["error"] = sqlite3_errmsg(db);
            return result;
        }

        // カラム情報を取得
        int columnCount = sqlite3_column_count(stmt);
        std::vector<std::string> columnNames;

        for (int i = 0; i < columnCount; i++) {
            columnNames.push_back(sqlite3_column_name(stmt, i));
        }

        // データ行を取得
        json rows = json::array();

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json row;

            for (int i = 0; i < columnCount; i++) {
                const std::string colName = columnNames[i];
                int colType = sqlite3_column_type(stmt, i);

                switch (colType) {
                    case SQLITE_INTEGER:
                        row[colName] = sqlite3_column_int64(stmt, i);
                        break;
                    case SQLITE_FLOAT:
                        row[colName] = sqlite3_column_double(stmt, i);
                        break;
                    case SQLITE_TEXT:
                        row[colName] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                        break;
                    case SQLITE_BLOB:
                        // BLOBデータはBase64などに変換するか、文字列として扱う
                        row[colName] = "[BLOBデータ]";
                        break;
                    case SQLITE_NULL:
                    default:
                        row[colName] = nullptr;
                        break;
                }
            }

            rows.push_back(row);
        }

        sqlite3_finalize(stmt);

        result["columns"] = columnNames;
        result["row_count"] = rows.size();
        result["data"] = rows;

        return result;
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

    EmbedData getOneItem(std::string table_name) {
        EmbedData ret;

        std::string sql =
            "SELECT id, content, embeddings FROM " + table_name + " LIMIT 1;";

        sqlite3_stmt* stmt = prepare(sql.c_str());
        int count = 0;
        std::vector<EmbedData> items;
        EmbedData data;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            data.id      = col_text(stmt, 0);
            data.content      = col_text(stmt, 1);
            std::string emb_str = col_text(stmt, 2);
            json j1 = json::parse(emb_str);
            auto vec = j1;
            int vlength = sizeof(vec) / sizeof(vec[0]);
            //std::cout << "vlen=" << vec.size() << std::endl;
            data.embedding    = vec.get<std::vector<float>>();
        }
        sqlite3_finalize(stmt);
        ret = data;
        return ret;
    }

    std::vector<EmbedData> getTableItems(std::string table_name) {
        std::string sql =
            "SELECT id, content, embeddings FROM " + table_name + ";";

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

    int get_count(std::string table_name) {
        int ret = 0;
        std::string sql =
            "SELECT COUNT(*) FROM " + table_name + ";";

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

    bool validate_vec_len(std::string table_name, std::string vec_str) 
    {
        bool ret = false;
        try {
            json j1 = json::parse(vec_str);
            std::cout << "size: " << j1.size() << '\n';

            int count =  get_count(table_name);
            std::cout << "count=" << count << "\n";
            if(count == 0){
                std::cout << "error, documet none"  << "\n";
                return true;
            }
            auto embedding = j1;
            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            std::cout << "embedding.vlen=" << embedding.size() << std::endl; 
            
            EmbedData one_item = getOneItem(table_name);
            std::cout << "one_item.embedding.vlen=" << one_item.embedding.size() << std::endl; 
            if(embedding.size() != one_item.embedding.size()){
                return ret;
            }
            //std::cout << "result_items.vlen=" << result_items.size() << std::endl;
            return true;
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return ret;
        }
        return ret;
    }  

    std::string getTableList(std::string table_name, std::string vec_str, int limit) {
        std::string ret = "";
        try {
            json j1 = json::parse(vec_str);
            std::cout << "size: " << j1.size() << '\n';

            int count =  get_count(table_name);
            //cout << "count=" << count << "\n";
            if(count == 0){
                std::cout << "error, documet none"  << "\n";
                return "[]";
            }
            auto embedding = j1;
            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            std::cout << "embedding.vlen=" << embedding.size() << std::endl;            

            std::vector<EmbedData> items = getTableItems(table_name);
            std::vector<ResultEmbed> result_items;
            for (const auto& data : items) {
                std::string id = data.id;
                std::vector<float> vec = data.embedding;
                int vlength = sizeof(vec) / sizeof(vec[0]);

                float distance = cosine_similarity(embedding, vec);
                std::cout << "distance=" << distance << std::endl;            
                ResultEmbed res_item;
                res_item.id = id;
                res_item.embedding = vec;
                res_item.content = data.content;
                res_item.distance = distance;
                if(distance > 0.5) {
                    result_items.push_back(res_item);
                }
            }
            std::cout << "result_items.vlen=" << result_items.size() << std::endl;

            //sort
            std::sort(result_items.begin(), result_items.end(),
                [](const ResultEmbed& a, const ResultEmbed& b) {
                    return a.distance > b.distance;
                }
            );   
            std::vector<SearchDbItem> out_items;
            for (const auto& item : result_items) {
                if (out_items.size() < limit) {
                    std::cout << "distance=" << item.distance
                        << ", id=" << item.id << std::endl;
                    SearchDbItem db_row;
                    db_row.id = item.id;
                    db_row.distance = item.distance;
                    db_row.content = item.content;
                    out_items.push_back(db_row);
                }        
            }
            json j2 = out_items;
            std::string json_str = j2.dump();
            std::cout << json_str << std::endl;            
            ret = json_str;
            return ret;
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return ret;
        }
        return ret;
    }     
        
    // JSON形式で一覧取得（API用）
    std::string getTodosAsJSON() {
        std::stringstream json;
        json << "[";
        
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return "[]";
        }
        
        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json << ",";
            first = false;
            
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            json << "{"
                 << "\"id\":" << id << ","
                 << "\"title\":\"" << (title ? reinterpret_cast<const char*>(title) : "") << "\","
                 << "\"description\":\"" << (description ? reinterpret_cast<const char*>(description) : "") << "\","
                 << "\"created_at\":\"" << (created_at ? reinterpret_cast<const char*>(created_at) : "") << "\","
                 << "\"completed\":" << (completed ? "true" : "false")
                 << "}";
        }
        
        sqlite3_finalize(stmt);
        json << "]";
        return json.str();
    }
};