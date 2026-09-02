#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "AppState.hpp"
#include "UI_InputForm.hpp"
#include "UI_StackViewer.hpp"

void apply_theme(bool is_dark_mode) {
    if (is_dark_mode) ImGui::StyleColorsDark();
    else ImGui::StyleColorsLight();
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

    AppState state;
    UI_InputForm input_form;
    UI_StackViewer stack_viewer;

    apply_theme(state.config.is_dark_mode);
    io.FontGlobalScale = state.config.font_scale;

    float base_scale = 1.8f;
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    if (xscale > 0.0f) base_scale = xscale;
#endif

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(base_scale);
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;

    float font_size = 18.0f * base_scale;
    static const ImWchar glyph_ranges[] = {
        0x0020, 0x00FF, 0x3000, 0x30FF, 0x31F0, 0x31FF, 0x4E00, 0x9FAF, 0xFF00, 0xFFEF, 0,
    };

    ImFont* custom_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", font_size, NULL, glyph_ranges);
    if (!custom_font) custom_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", font_size, NULL, glyph_ranges);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool open_modal_requested = false;
    bool modal_is_open = false;
    bool force_close = false;

    while (!force_close) {
        glfwPollEvents();

        // 非同期保存の進捗チェック
        state.update_async_save();

        // ×ボタンフック
        if (glfwWindowShouldClose(window) && !force_close) {
            glfwSetWindowShouldClose(window, GLFW_FALSE); // 閉じる処理を一旦ストップ
            
            if (state.is_dirty || state.is_saving) {
                if (!modal_is_open && !open_modal_requested) {
                    std::cout << "[DEBUG] Window close requested -> Opening Modal" << std::endl;
                    open_modal_requested = true;
                }
            } else {
                std::cout << "[DEBUG] Clean state -> Closing window" << std::endl;
                force_close = true;
            }
        }

        // ショートカット
        if (io.KeyCtrl && !state.is_saving) {
            if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                state.redo();
            } else if (!io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                state.undo();
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGui::Begin("Main Window", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (custom_font) ImGui::PushFont(custom_font);

        // ヘッダー情報
        if (state.is_saving) {
            ImGui::BeginDisabled();
            ImGui::Button(" Saving... ", ImVec2(160.0f * base_scale, 0));
            ImGui::EndDisabled();
        } else {
            if (state.is_dirty) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Unsaved]");
                ImGui::SameLine();
            }
            if (ImGui::Button(" Save & Commit ", ImVec2(160.0f * base_scale, 0))) {
                state.start_save_and_git("manual save");
            }
        }

        if (!state.status_msg.empty()) {
            ImGui::SameLine();
            if (state.is_saving) {
                static float timer = 0.0f;
                timer += ImGui::GetIO().DeltaTime;
                int dots = (int)(timer * 4.0f) % 4;
                std::string dots_str = std::string(dots, '.');
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s%s", state.status_msg.c_str(), dots_str.c_str());
            } else {
                ImGui::TextDisabled("(%s)", state.status_msg.c_str());
            }
        }

        ImGui::Spacing();

        if (ImGui::BeginTabBar("MainTabBar")) {
            if (ImGui::BeginTabItem("Stack App")) {
                ImGui::Spacing();
                ImGui::BeginChild("MainScrollView", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                input_form.Render(state, base_scale);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                stack_viewer.Render(state, base_scale);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Spacing();
                ImGui::Text("Appearance");
                ImGui::Separator();
                
                if (ImGui::RadioButton("Dark Mode", state.config.is_dark_mode)) {
                    state.config.is_dark_mode = true;
                    apply_theme(true);
                    state.save_config();
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Light Mode", !state.config.is_dark_mode)) {
                    state.config.is_dark_mode = false;
                    apply_theme(false);
                    state.save_config();
                }

                ImGui::Spacing();
                if (ImGui::SliderFloat("Font Scale", &state.config.font_scale, 0.7f, 2.0f, "%.2fx")) {
                    io.FontGlobalScale = state.config.font_scale;
                    state.save_config();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // --- モーダル制御 ---
        if (open_modal_requested) {
            std::cout << "[DEBUG] Executing OpenPopup" << std::endl;
            ImGui::OpenPopup("Save Changes?");
            open_modal_requested = false;
            modal_is_open = true;
        }

        if (ImGui::BeginPopupModal("Save Changes?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            modal_is_open = true;

            if (state.is_saving) {
                ImGui::Text("Saving and committing in background...\nPlease wait.");
            } else if (!state.is_dirty && state.status_msg.find("Saved") != std::string::npos) {
                // 保存完了を検知して終了
                std::cout << "[DEBUG] Save complete inside modal -> Force exit" << std::endl;
                force_close = true;
                modal_is_open = false;
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::Text("You have unsaved changes.\nDo you want to save before exiting?\n\n");
                
                if (ImGui::Button("Yes (Save & Exit)", ImVec2(160 * base_scale, 0))) {
                    std::cout << "[DEBUG] Clicked 'Yes (Save & Exit)' -> Starting Async Save" << std::endl;
                    state.start_save_and_git("exit save");
                }
                ImGui::SameLine();
                if (ImGui::Button("No (Exit Without Saving)", ImVec2(200 * base_scale, 0))) {
                    std::cout << "[DEBUG] Clicked 'No' -> Exiting" << std::endl;
                    force_close = true;
                    modal_is_open = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100 * base_scale, 0))) {
                    std::cout << "[DEBUG] Clicked 'Cancel'" << std::endl;
                    modal_is_open = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        } else {
            modal_is_open = false;
        }

        if (custom_font) ImGui::PopFont();
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        if (state.config.is_dark_mode) glClearColor(0.09f, 0.09f, 0.10f, 1.0f);
        else glClearColor(0.90f, 0.90f, 0.92f, 1.0f);
        
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    std::cout << "[DEBUG] Terminating app" << std::endl;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}