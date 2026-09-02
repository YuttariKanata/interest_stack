#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "single_include/json.hpp"

using json = nlohmann::json;

std::string get_current_time_iso() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string get_current_date() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
    return ss.str();
}

std::vector<std::string> parse_tags(const std::string& tags_raw) {
    std::vector<std::string> tags;
    if (tags_raw.empty()) return tags;

    std::stringstream ss(tags_raw);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t first = item.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = item.find_last_of(" \t\r\n");
            std::string trimmed = item.substr(first, (last - first + 1));
            if (!trimmed.empty()) {
                tags.push_back(trimmed);
            }
        }
    }
    return tags;
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 850, "interest_stack - Push", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // 高DPI検出
    float base_scale = 1.8f;
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    if (xscale > 0.0f) base_scale = xscale;
#endif

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(base_scale);
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.FramePadding = ImVec2(8.0f * base_scale, 6.0f * base_scale);

    // 日本語フォントロード
    float font_size = 18.0f * base_scale;
    static const ImWchar glyph_ranges[] = {
        0x0020, 0x00FF,
        0x3000, 0x30FF,
        0x31F0, 0x31FF,
        0x4E00, 0x9FAF,
        0xFF00, 0xFFEF,
        0,
    };

    ImFont* custom_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", font_size, NULL, glyph_ranges);
    if (custom_font == nullptr) {
        custom_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", font_size, NULL, glyph_ranges);
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    char buf_title[256] = "";
    char buf_context[2048] = "";
    char buf_link[512] = "";
    char buf_tags[256] = "";
    std::string status_msg = "";

    const std::string json_path = "../stack.json";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGui::Begin("Main Window", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (custom_font) ImGui::PushFont(custom_font);

        if (ImGui::BeginTabBar("MainTabBar")) {

            if (ImGui::BeginTabItem("Push Context")) {
                ImGui::Spacing();
                
                ImGui::Text("Title:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##title", buf_title, IM_ARRAYSIZE(buf_title));

                ImGui::Spacing();
                ImGui::Text("Context (Multi-line):");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextMultiline("##context", buf_context, IM_ARRAYSIZE(buf_context), ImVec2(-1, 200.0f * base_scale));

                ImGui::Spacing();
                ImGui::Text("Link (Optional):");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##link", buf_link, IM_ARRAYSIZE(buf_link));

                ImGui::Spacing();
                ImGui::Text("Tags (comma separated):");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##tags", buf_tags, IM_ARRAYSIZE(buf_tags));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("PUSH TO STACK (Ctrl+Enter)", ImVec2(-1, 50.0f * base_scale)) || 
                   (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    
                    if (std::string(buf_title).empty()) {
                        status_msg = "Error: Title is required.";
                    } else {
                        try {
                            std::ifstream ifs(json_path);
                            if (!ifs.is_open()) {
                                throw std::runtime_error("Cannot open stack.json");
                            }
                            json j;
                            ifs >> j;
                            ifs.close();

                            json new_item;
                            new_item["id"] = "frame-" + std::to_string(j["stack"].size());
                            new_item["title"] = std::string(buf_title);
                            new_item["context"] = std::string(buf_context);
                            new_item["link"] = std::string(buf_link);
                            new_item["pushed_at"] = get_current_date();
                            new_item["tags"] = parse_tags(std::string(buf_tags));

                            j["stack"].insert(j["stack"].begin(), new_item);
                            j["updated_at"] = get_current_time_iso();

                            std::ofstream ofs(json_path);
                            ofs << j.dump(4) << std::endl;
                            ofs.close();

                            std::ofstream msg_file("commit_msg.txt");
                            msg_file << "push: " << buf_title << std::endl;
                            msg_file.close();

                            std::string git_cmd = "cmd.exe /c \"cd /d .. && git add stack.json && git commit -F app/commit_msg.txt && git push\" > git_output.log 2>&1";
                            int res = std::system(git_cmd.c_str());

                            std::remove("commit_msg.txt");

                            if (res == 0) {
                                glfwSetWindowShouldClose(window, GLFW_TRUE);
                            } else {
                                std::ifstream log_file("git_output.log");
                                std::string log_content((std::istreambuf_iterator<char>(log_file)), std::istreambuf_iterator<char>());
                                if (log_content.empty()) {
                                    status_msg = "Git push failed (Exit code: " + std::to_string(res) + ")";
                                } else {
                                    status_msg = "Git push failed:\n" + log_content;
                                }
                            }
                        } catch (const std::exception& e) {
                            status_msg = std::string("Error: ") + e.what();
                        }
                    }
                }

                if (!status_msg.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", status_msg.c_str());
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (custom_font) ImGui::PopFont();

        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09f, 0.09f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}