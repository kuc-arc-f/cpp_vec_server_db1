#pragma once
#include <iostream>

class MyConfig {
private:

public:
    int EMBED_SIZE = 4096;

    explicit MyConfig(std::string str){}
    ~MyConfig() {}
};
