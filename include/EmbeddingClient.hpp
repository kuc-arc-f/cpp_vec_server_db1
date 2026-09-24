#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // JSONライブラリ（要インストール）

using json = nlohmann::json;

// コールバック関数：レスポンスデータを蓄積
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

class EmbeddingClient {
private:
    std::string api_key;
    std::string base_url;
    
public:
    EmbeddingClient(const std::string& key) 
        : api_key(key), base_url("https://openrouter.ai/api/v1/embeddings") {}
    
    // テキストを埋め込みベクトルに変換
    json embedContent(const std::string& text) {
        CURL* curl;
        CURLcode res;
        std::string response_string;
        
        // curl初期化
        curl = curl_easy_init();
        if(!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        // JSONリクエストボディの作成
        json request_body = {
            {"model", "qwen/qwen3-embedding-8b"},
            {"input", text}
        };
        
        std::string post_data = request_body.dump();
        
        // ヘッダーの設定
        struct curl_slist* headers = nullptr;
        std::string api_key_header = "Authorization: Bearer " + api_key;
        headers = curl_slist_append(headers, api_key_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // curlオプション設定
        curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        
        // SSL証明書検証（必要に応じて無効化も可能）
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // リクエスト実行
        res = curl_easy_perform(curl);
        
        // エラーチェック
        if(res != CURLE_OK) {
            std::string error_msg = "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            throw std::runtime_error(error_msg);
        }
        
        // HTTPレスポンスコードの取得
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        
        // レスポンスコードチェック
        if(http_code != 200) {
            throw std::runtime_error("HTTP Error " + std::to_string(http_code) + ": " + response_string);
        }
        
        // JSONパース
        try {
            return json::parse(response_string);
        } catch(const json::parse_error& e) {
            throw std::runtime_error("Failed to parse JSON response: " + std::string(e.what()));
        }
    }
    /*
    std::optional<std::vector<double>> getEmbedding(const json& j) {
        try {
            if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                auto& data_array = j["data"];
                
                if (data_array[0].contains("embedding") && data_array[0]["embedding"].is_array()) {
                    return data_array[0]["embedding"].get<std::vector<double>>();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting embedding: " << e.what() << std::endl;
        }
        return std::nullopt;
    }
    */

    // 埋め込みベクトルを抽出
    std::vector<float> extractEmbedding(const json& j) {
        std::vector<float> embedding;
        
        try {
            // レスポンスから配列を取得
            if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                auto& data_array = j["data"];
                
                if (data_array[0].contains("embedding") && data_array[0]["embedding"].is_array()) {
                    return data_array[0]["embedding"].get<std::vector<float>>();
                }
            }            
        } catch(const std::exception& e) {
            throw std::runtime_error("Failed to extract embedding: " + std::string(e.what()));
        }
        
        return embedding;
    }
    
};

std::vector<float> EmbeddingStart(std::string query) {
    std::vector<float> resp;
    // APIキーの設定（環境変数から取得推奨）
    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    if(!api_key_env) {
        std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
        return resp;
    }
    
    std::string api_key = api_key_env;
    
    // curlグローバル初期化
    curl_global_init(CURL_GLOBAL_ALL);
    
    try {
        // クライアント作成
        EmbeddingClient client(api_key);
        
        // 埋め込みを取得するテキスト
        std::string text = query;
        
        //std::cout << "Sending request to Gemini Embedding API..." << std::endl;
        //std::cout << "Text: " << text << std::endl << std::endl;
        
        // API呼び出し
        json response = client.embedContent(text);
        
        // レスポンス全体を表示（デバッグ用）
        //std::cout << "Full Response:" << std::endl;
        //std::cout << response.dump(2) << std::endl << std::endl;
        
        // 埋め込みベクトルを抽出
        std::vector<float> embedding = client.extractEmbedding(response);
        return embedding;            
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        curl_global_cleanup();
        return resp;
    }
    
    curl_global_cleanup();
    return resp;
}