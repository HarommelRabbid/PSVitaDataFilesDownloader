// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vita2d.h>
#include <fstream>
#include <imgui_vita2d/imgui_vita.h>

#include "utils.h++"
#include "dl.h"
#include "include/json.hpp"
using json = nlohmann::json;

bool info_window_open = false;

typedef enum {
    dark_theme,
    light_theme,
    classic_theme
} Themes;

Themes theme = dark_theme;

json defaultConfig = {
    {"theme", "dark"},
    {"update_list_on_start", true}
};

void SaveConfig(const json& config) {
    std::ofstream outFile("ux0:data/DataFiles/config.json");
    if (outFile.is_open()) {
        outFile << config.dump(4); // pretty-print with 4-space indent
        outFile.close();
    }else sceClibPrintf("Failed to create config file.\n");
}

json LoadOrCreateConfig() {
    json config;

    if (std::filesystem::exists("ux0:data/DataFiles/config.json")) {
        std::ifstream inFile("ux0:data/DataFiles/config.json");
        try {
            inFile >> config;
        } catch (const std::exception& e) {
            sceClibPrintf("Config load error: %s\n", e.what());
            config = defaultConfig;
            SaveConfig(config);
        }
    } else {
        config = defaultConfig;
        SaveConfig(config);
    }

    return config;
}

int main(){
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

    if(!Utils::FileOrPathExists("ux0:data/DataFiles")) sceIoMkdir("ux0:data/DataFiles", 0777);

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
    SceNetInitParam netInitParam;
	netInitParam.memory = malloc(1*1024*1024);
	netInitParam.size = 1*1024*1024;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);
	sceNetCtlInit();
	sceHttpInit(1*1024*1024);
	sceSslInit(1*1024*1024);

    json config = LoadOrCreateConfig();

    if(config["update_list_on_start"]){
        dl_topath_nothread("https://raw.githubusercontent.com/NFSHubster/PS-Vita-Data-Files/refs/heads/main/list_hbs_json.php", "ux0:data/DataFiles/datafiles.json");
    }

    std::ifstream f("ux0:data/DataFiles/datafiles.json");
    if (!f.is_open()) {
        printf("Failed to open \"ux0:data/DataFiles/datafiles.json\"\n");
    }
    json datafiles = json::parse(f);

    ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVita2D_Init();
    ImGui_ImplVita2D_TouchUsage(true);
    ImGui_ImplVita2D_UseRearTouch(false);
    ImGui_ImplVita2D_UseIndirectFrontTouch(false);
    ImGui_ImplVita2D_GamepadUsage(true);
    ImGui_ImplVita2D_MouseStickUsage(false);
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::GetIO().MouseDrawCursor = false;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));

    if(config["theme"] == "light"){
        theme = light_theme;
        ImGui::StyleColorsLight();
    }else if(config["theme"] == "classic"){
        theme = classic_theme;
        ImGui::StyleColorsClassic();
    }

    while(true){
        vita2d_start_drawing();
        vita2d_clear_screen();

        ImGui_ImplVita2D_NewFrame();

        if(ImGui::BeginMainMenuBar()){
            ImGui::TextDisabled("PS Vita Ports Downloader");
            if(ImGui::BeginMenu("Settings")){
                if(ImGui::MenuItem("Update list every time the app starts", nullptr, config["update_list_on_start"])){
                    config["update_list_on_start"] = !config["update_list_on_start"];
                    SaveConfig(config);
                }
                if(ImGui::BeginMenu("Theme")){
                    if(ImGui::MenuItem("Dark", nullptr, theme == dark_theme) && theme != dark_theme){
                        theme = dark_theme;
                        config["theme"] = "dark";
                        ImGui::StyleColorsDark();
                        SaveConfig(config);
                    }
                    if(ImGui::MenuItem("Light", nullptr, theme == light_theme) && theme != light_theme){
                        theme = light_theme;
                        config["theme"] = "light";
                        ImGui::StyleColorsLight();
                        SaveConfig(config);
                    }
                    if(ImGui::MenuItem("Classic", nullptr, theme == classic_theme) && theme != classic_theme){
                        theme = classic_theme;
                        config["theme"] = "classic";
                        ImGui::StyleColorsClassic();
                        SaveConfig(config);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("About")){
                ImGui::Text("PS Vita Ports Downloader by Harommel OddSock");
                ImGui::Text("ImGui version %s", ImGui::GetVersion());
                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("Exit")) break;
            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 21));
        ImGui::SetNextWindowSize(ImVec2(960/2, 544-21));
        if(ImGui::Begin("PS Vita Data Files Downloader", nullptr, ImGuiWindowFlags_NoCollapse + ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoMove + ImGuiWindowFlags_NoTitleBar)){
            for(const auto& item : datafiles){
                if(ImGui::Button(item["name"].get<std::string>().c_str(), ImVec2(-1, 0))) printf("Placeholder");
            }
        }
        ImGui::End();

        ImGui::Render();
	    ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    ImGui_ImplVita2D_Shutdown();
	ImGui::DestroyContext();

    sceSslTerm();
	sceHttpTerm();
	sceNetCtlTerm();
	sceNetTerm();
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);

    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}