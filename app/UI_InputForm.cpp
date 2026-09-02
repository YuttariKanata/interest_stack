#include "UI_InputForm.hpp"
#include "imgui.h"

void UI_InputForm::Render(AppState& state, float base_scale) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    ImGui::Text("Title:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##title", buf_title, IM_ARRAYSIZE(buf_title));

    ImGui::Spacing();
    ImGui::Text("Context:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextMultiline("##context", buf_context, IM_ARRAYSIZE(buf_context), ImVec2(-1, 80.0f * base_scale));

    ImGui::Spacing();
    ImGui::Text("Link (Optional):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##link", buf_link, IM_ARRAYSIZE(buf_link));

    ImGui::Spacing();

    // 下向き矢印の大きな PUSH ボタン（スマホアプリ風アイコンボタン）
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    
    if (ImGui::Button("  v  PUSH TO STACK  v  ", ImVec2(-1, 40.0f * base_scale))) {
        if (std::string(buf_title).empty()) {
            state.status_msg = "Error: Title is required.";
        } else {
            state.push_item(buf_title, buf_context, buf_link);
            // 入力バッファをクリア
            buf_title[0] = '\0';
            buf_context[0] = '\0';
            buf_link[0] = '\0';
        }
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}