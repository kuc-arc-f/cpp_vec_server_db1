#pragma once
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "EmbeddingClient.hpp"
#include "db_add.hpp"
#include "db_search.hpp"
#include "my_config.hpp"
#include "openrouter_client.hpp"

using json = nlohmann::json;

const std::string DB_PATH = "example.db";
const std::string DATA_PATH = "./data";

// 1ファイル分のデータを保持する構造体
struct TextFile {
    std::string filename;
    std::vector<std::string> lines;
};

// .txt ファイルを読み込んで行を返す
TextFile loadTextFile(const std::filesystem::path& filepath) {
    TextFile tf;
    tf.filename = filepath.filename().string();

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        std::cerr << "[警告] ファイルを開けません: " << filepath << "\n";
        return tf;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        tf.lines.push_back(line);
    }
    return tf;
}

class MyRag {
private:
    std::string m_name;

    public:
    explicit MyRag(std::string str){}

    ~MyRag() {}

    /**
    *
    * @param
    *
    * @return
    */
    int ebmed(std::string query){
        int ret = 0;

        auto embeddings = EmbeddingStart(query);
        //std::cout << "vlen=" << embeddings.size() << std::endl;
        try{

            auto vec = embeddings;

            double v1 = vec[0];
            double v2 = vec[1];        
            //std::cout << v1 << ", " << v2 << std::endl;

            DbAdd app(DB_PATH);
            app.add_embed(vec, query);

        } catch (const std::exception& e) {
            //std::cout << "Error , main" << std::endl;
            return 1;
        }        
        
        return 0;
    }    

    // 読み込んだデータを表示する
    void addTextFiles(const std::vector<TextFile>& files) {
        for (const auto& tf : files) {
            std::cout << "========================================\n";
            std::cout << "ファイル名: " << tf.filename << "\n";
            std::cout << "行数      : " << tf.lines.size() << "\n";
            std::cout << "----------------------------------------\n";
            std:string target = "";
            for (size_t i = 0; i < tf.lines.size(); ++i) {
                //std::cout << "[" << i + 1 << "] " << tf.lines[i] << "\n";
                std::string tmp = tf.lines[i] + "\n";
                target.append(tmp);
            }
            std::cout <<  target << "\n";
            int resp = ebmed(target);
            std::cout << "resp=" << resp << "\n";
        }
        std::cout << "========================================\n";
    }

    std::string rag_search_handler(std::string query){
        try{
            std::string ret = "";
            // API_KEYを環境変数から取得（または直接設定）
            const char* api_key = std::getenv("OPENROUTER_API_KEY");
            if (api_key != nullptr) {
                //std::cout << "api_key:" << api_key << std::endl;
            }else{
                std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
                std::cerr << "Please set it with: export OPENROUTER_API_KEY=your_api_key_here" << std::endl;
                return ret;
            }      
            const char* model_name = std::getenv("OPENROUTER_MODEL");
            if (!model_name) {
                std::cerr << "Error: OPENROUTER_MODEL environment variable not set" << std::endl;
                return ret;
            }      

            std::string dirPath = DATA_PATH;

            auto embeddings = EmbeddingStart(query);
            //std::cout << "vlen=" << embeddings.size() << std::endl;
            auto vec = embeddings;
            DbSearch app(DB_PATH);
            if(app.search_embed_size(vec) == false){
                return 0;
            }
            std::string resp_str = app.rag_search(vec);
            
            std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
            if(resp_str.empty()){
                out_str.append("user query: ");
                out_str.append(query);
                out_str.append(" \n");
            }else{
                out_str.append("context:");
                out_str.append(resp_str);
                out_str.append("\n user query: ");
                out_str.append(query);
                out_str.append(" \n");
            }
            //std::cout << out_str  << std::endl; 
            OpenRouterClient client(api_key);

            // チャットリクエストの送信
            auto response = client.sendChatCompletion(
                model_name,
                out_str,
                1.0,
                2000
            );

            if (response.has_value()) {
                //std::cout << "Response: " << response.value() << std::endl;
                auto outStr = response.value();
                ret = outStr;
                return ret;
            } else {
                std::cerr << "Failed to get response from API" << std::endl;
                return ret;
            }

            return ret;
        } catch (const std::exception& e) {
            std::cout << "Error , main" << std::endl;
        }  
    }


};
