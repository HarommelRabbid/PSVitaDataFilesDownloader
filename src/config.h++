// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#pragma once

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>

#include "include/json.hpp"
using json = nlohmann::json;

json defaultConfig = {
    {"theme", "dark"},
    {"update_list_on_start", true}
};

namespace Config{
    void Save(const json& config) {
        std::ofstream outFile("ux0:data/DataFiles/config.json");
        if (outFile.is_open()) {
            outFile << config.dump(4); // pretty-print with 4-space indent
            outFile.close();
        }else sceClibPrintf("Failed to create config file.\n");
    }

    json LoadOrCreate() {
        json config;

        if (std::filesystem::exists("ux0:data/DataFiles/config.json")) {
            std::ifstream inFile("ux0:data/DataFiles/config.json");
            try {
                inFile >> config;
            } catch (const std::exception& e) {
                sceClibPrintf("Config load error: %s\n", e.what());
                config = defaultConfig;
                Config::Save(config);
            }
        } else {
            config = defaultConfig;
            Config::Save(config);
        }

        return config;
    }
}