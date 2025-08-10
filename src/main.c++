// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vita2d.h>
#include <fstream>
#include <string>
#include <imgui_vita2d/imgui_vita.h>

#include "utils.h++"
#include "config.h++"
#include "dl.h"
#include "include/json.hpp"
using json = nlohmann::json;

bool info_window_open = false;
bool dl_no_data = false, dl_no_vpk = false;
std::string info_window_check;

SceCtrlData pad, oldpad;

typedef enum {
    dark_theme,
    light_theme,
    classic_theme
} Themes;

Themes theme = dark_theme;

int main(){
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

    if(!Utils::FileOrPathExists("ux0:data/DataFiles")) sceIoMkdir("ux0:data/DataFiles", 0777);
    if(!Utils::FileOrPathExists("ux0:data/DataFiles/screenshots")) sceIoMkdir("ux0:data/DataFiles/screenshots", 0777);

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
    curl_global_init(CURL_GLOBAL_ALL);

    json config = Config::LoadOrCreate();

    if(config["update_list_on_start"]){
        dl_topath_nothread("https://raw.githubusercontent.com/NFSHubster/PS-Vita-Data-Files/refs/heads/main/list_hbs_json.php", "ux0:data/DataFiles/datafiles.json");
    }

    std::ifstream f("ux0:data/DataFiles/datafiles.json");
    if (!f.is_open()) {
        sceClibPrintf("Failed to open \"ux0:data/DataFiles/datafiles.json\"\n");
        sceKernelExitProcess(0);
    }
    json datafiles = json::parse(f);

    ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVita2D_Init();
    ImGui_ImplVita2D_TouchUsage(false);
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
        sceCtrlPeekBufferPositive(0, &pad, 1);
        check_dl_thd_for_free();
        vita2d_start_drawing();
        vita2d_clear_screen();

        ImGui_ImplVita2D_NewFrame();

        if(ImGui::BeginMainMenuBar()){
            ImGui::TextDisabled("PS Vita Data Files Downloader");
            if(ImGui::BeginMenu("Settings")){
                if(ImGui::MenuItem("Update list every time the app starts", nullptr, config["update_list_on_start"])){
                    config["update_list_on_start"] = !config["update_list_on_start"];
                    Config::Save(config);
                }
                if(ImGui::BeginMenu("Theme")){
                    if(ImGui::MenuItem("Dark", nullptr, theme == dark_theme) && theme != dark_theme){
                        theme = dark_theme;
                        config["theme"] = "dark";
                        ImGui::StyleColorsDark();
                        Config::Save(config);
                    }
                    if(ImGui::MenuItem("Light", nullptr, theme == light_theme) && theme != light_theme){
                        theme = light_theme;
                        config["theme"] = "light";
                        ImGui::StyleColorsLight();
                        Config::Save(config);
                    }
                    if(ImGui::MenuItem("Classic", nullptr, theme == classic_theme) && theme != classic_theme){
                        theme = classic_theme;
                        config["theme"] = "classic";
                        ImGui::StyleColorsClassic();
                        Config::Save(config);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("About")){
                ImGui::Text("PS Vita Data Files Downloader by Harommel OddSock");
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
                if(ImGui::Button(item["name"].get<std::string>().c_str(), ImVec2(-1, 0))){
                    info_window_open = true;
                    info_window_check = item["name"].get<std::string>();
                }
                if(info_window_open && item["name"] == info_window_check){ // preventing window from appearing for each entry
                    ImGui::PopStyleVar();
                    ImGui::SetNextWindowPos(ImVec2(480-(480/2), 272-(272/2)));
                    ImGui::SetNextWindowSize(ImVec2(480, 272));
                    ImGui::SetNextWindowFocus();
                    if(ImGui::Begin(item["name"].get<std::string>().c_str(), nullptr, ImGuiWindowFlags_NoCollapse + ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoMove + ImGuiWindowFlags_Modal)){
                        ImGui::TextWrapped(item["description"].get<std::string>().c_str());
                        ImGui::Text(item["date"].get<std::string>().c_str());
                        ImGui::Text(item["titleid"].get<std::string>().c_str());
                        ImGui::Text(std::string(std::string(("Size: ")) + item["size"].get<std::string>()).c_str());
                        if(item.contains("data_size") || (item.contains("data") && item["data"].get<std::string>() != "")) ImGui::Text(std::string(std::string(("Data files size: ")) + item["data_size"].get<std::string>()).c_str());
                        ImGui::Separator();
                        if(ImGui::Button("Download .VPK", ImVec2(-1, 0))) {
                            std::string path = "ux0:data/DataFiles/" + item["name"].get<std::string>() + ".vpk";
                            dl_topath(item["url"].get<std::string>().c_str(), path.c_str());
                            dl_no_vpk = false, dl_no_data = true;
                        }
                        if(is_dl && dl_no_data){
                            sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
                            ImGui::SetNextWindowPos(ImVec2((960/2)-200, (544/2)-50));
                            ImGui::SetNextWindowSize(ImVec2(400, 100));
                            ImGui::SetNextWindowFocus();
                            if(ImGui::Begin("Progress", nullptr, ImGuiWindowFlags_NoCollapse + ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoMove + ImGuiWindowFlags_NoTitleBar + ImGuiWindowFlags_NoNav + ImGuiWindowFlags_NoNavFocus + ImGuiWindowFlags_NoNavInputs)){
                                ImVec2 textsize = ImGui::CalcTextSize("Downloading .VPK...");
                                ImGui::SetCursorPos(ImVec2((400 - textsize.x)/2, 20));
                                ImGui::Text("Downloading .VPK...");
                                ImGui::SetCursorPos(ImVec2(100, 60));
                                ImGui::ProgressBar(dl_progress / 100, ImVec2(200, 0));
                            }
                            ImGui::End();
                        }
                        if(item.contains("data") || (item.contains("data") && item["data"].get<std::string>() != "")){
                            if(ImGui::Button("Download Data Files", ImVec2(-1, 0))){
                                std::string path = "ux0:data/DataFiles/" + item["name"].get<std::string>() + ".zip";
                                dl_topath(item["data"].get<std::string>().c_str(), path.c_str());
                                dl_no_vpk = true, dl_no_data = false;
                            }
                            if(is_dl && dl_no_vpk){
                                sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
                                ImGui::SetNextWindowPos(ImVec2((960/2)-200, (544/2)-50));
                                ImGui::SetNextWindowSize(ImVec2(400, 100));
                                ImGui::SetNextWindowFocus();
                                if(ImGui::Begin("Progress", nullptr, ImGuiWindowFlags_NoCollapse + ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoMove + ImGuiWindowFlags_NoTitleBar + ImGuiWindowFlags_NoNav + ImGuiWindowFlags_NoNavFocus + ImGuiWindowFlags_NoNavInputs)){
                                    ImVec2 textsize = ImGui::CalcTextSize("Downloading data files...");
                                    ImGui::SetCursorPos(ImVec2((400 - textsize.x)/2, 20));
                                    ImGui::Text("Downloading data files...");
                                    ImGui::SetCursorPos(ImVec2(100, 60));
                                    ImGui::ProgressBar(dl_progress / 100, ImVec2(200, 0));
                                }
                                ImGui::End();
                            }
                        }
                        ImGui::Button("View Screenshots", ImVec2(-1, 0));
                        if((pad.buttons & SCE_CTRL_CIRCLE) && !(oldpad.buttons & SCE_CTRL_CIRCLE) && !is_dl) info_window_open = false;
                    }
                    ImGui::End();
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
                }
            }
        }
        ImGui::End();

        if((pad.buttons & SCE_CTRL_START) && !(oldpad.buttons & SCE_CTRL_START) && (!is_dl || dl_no_data || dl_no_vpk)) break;

        ImGui::Render();
	    ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

        vita2d_end_drawing();
        vita2d_swap_buffers();
        oldpad = pad;
    }

    ImGui_ImplVita2D_Shutdown();
	ImGui::DestroyContext();

	curl_global_cleanup();
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