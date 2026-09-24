#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

class BackupDb {
private:
    sqlite3* db;
    std::mutex mtx;
    
public:
    BackupDb(const std::string& dbPath = "todos.db") {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
        }
    }
    
    ~BackupDb() {
        if (db) {
            sqlite3_close(db);
        }
    }    

    bool executeSql(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cout << "error, sqlite3_prepare_v2" << std::endl;
            return false;
        }        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
    
    // 登録（INSERT）
    bool insertTodo(const std::string& title, const std::string& description = "") {
        std::string sql = "INSERT INTO todos (title, description) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
    
    // 一覧取得（SELECT）
    std::vector<std::string> getAllTodos() {
        std::vector<std::string> todos;
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return todos;
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            std::stringstream ss;
            ss << "ID: " << id 
               << ", Title: " << (title ? reinterpret_cast<const char*>(title) : "")
               << ", Description: " << (description ? reinterpret_cast<const char*>(description) : "")
               << ", Created: " << (created_at ? reinterpret_cast<const char*>(created_at) : "")
               << ", Completed: " << (completed ? "Yes" : "No");
            todos.push_back(ss.str());
        }
        
        sqlite3_finalize(stmt);
        return todos;
    }
    
    // 削除（DELETE）
    bool deleteTodo(int id) {
        std::string sql = "DELETE FROM todos WHERE id = ?;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_int(stmt, 1, id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
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