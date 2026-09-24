// openrouter_client.hpp
#ifndef OPENROUTER_CLIENT_HPP
#define OPENROUTER_CLIENT_HPP

#include <string>
#include <optional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;


class OpenRouterClient {
private:
    std::string api_key_;
    std::string api_url_ = "https://openrouter.ai/api/v1/chat/completions";

public:
    explicit OpenRouterClient(const std::string& api_key)
        : api_key_(api_key) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~OpenRouterClient() {
        curl_global_cleanup();
    }
    // APIキーを設定
    void setApiKey(const std::string& api_key) {
        api_key_ = api_key;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        std::string* response = static_cast<std::string*>(userp);
        response->append(static_cast<char*>(contents), total_size);
        return total_size;
    }    

    std::optional<std::string> sendChatCompletion(
        const std::string& model,
        const std::string& user_message,
        double temperature,
        int max_tokens) {
        
        if (api_key_.empty()) {
            std::cerr << "Error: API_KEY is not set" << std::endl;
            return std::nullopt;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Error: Failed to initialize curl" << std::endl;
            return std::nullopt;
        }

        std::string response_string;
        std::string error_string;

        // JSONペイロードの作成
        json payload = {
            {"model", model},
            {"messages", json::array({
                {{"role", "user"}, {"content", user_message}}
            })},
            {"max_tokens", max_tokens}
        };
        //{"temperature", temperature},

        std::string json_payload = payload.dump();

        // curlオプションの設定
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, api_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_payload.length());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_string.data());

        // SSL証明書の検証を有効にする（本番環境では推奨）
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // リクエスト実行
        CURLcode res = curl_easy_perform(curl);

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "Error: curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return std::nullopt;
        }

        if (response_code != 200) {
            std::cerr << "Error: HTTP response code: " << response_code << std::endl;
            std::cerr << "Response: " << response_string << std::endl;
            return std::nullopt;
        }

        // レスポンスからcontentを抽出
        try {
            json response_json = json::parse(response_string);
            if (response_json.contains("choices") && 
                response_json["choices"].is_array() && 
                !response_json["choices"].empty()) {
                
                auto& choice = response_json["choices"][0];
                if (choice.contains("message") && 
                    choice["message"].contains("content")) {
                    return choice["message"]["content"].get<std::string>();
                }
            }
            std::cerr << "Error: Unexpected response format" << std::endl;
            return std::nullopt;
        } catch (const json::parse_error& e) {
            std::cerr << "Error: Failed to parse JSON response: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

};

#endif // OPENROUTER_CLIENT_HPP