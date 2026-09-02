#include "UI_StackViewer.hpp"
#include "imgui.h"
#include <cstdio>
#include <string>

void UI_StackViewer::Render(AppState& state, float base_scale) {
    if (!state.stack_data.contains("stack") || !state.stack_data["stack"].is_array()) return;

    auto& stack_arr = state.stack_data["stack"];

    for (size_t i = 0; i < stack_arr.size(); ++i) {
        auto& item = stack_arr[i];
        std::string title = item.value("title", "No Title");
        std::string context = item.value("context", "");
        std::string link = item.value("link", "");
        std::string date = item.value("pushed_at", "");

        ImGui::PushID(static_cast<int>(i));

        ImVec2 start_pos = ImGui::GetCursorScreenPos();
        float avail_width = ImGui::GetContentRegionAvail().x;
        float padding_x = 12.0f * base_scale;
        float padding_y = 10.0f * base_scale;
        float menu_btn_width = 45.0f * base_scale;

        // --- 1. テキストを描画して高さを自動計算 ---
        ImGui::SetCursorScreenPos(ImVec2(start_pos.x + padding_x, start_pos.y + padding_y));

        ImGui::BeginGroup();
        // ... ボタンと被らないようにテキストを自動折り返し
        ImGui::PushTextWrapPos(start_pos.x + avail_width - menu_btn_width);

        ImGui::TextUnformatted(title.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%s)", date.c_str());

        if (!context.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", context.c_str());
        }

        ImGui::PopTextWrapPos();
        ImGui::EndGroup();

        // 描画されたテキストの高さからカードの高さを決定
        float card_height = ImGui::GetItemRectSize().y + padding_y * 2.0f;
        ImVec2 end_pos = ImVec2(start_pos.x + avail_width, start_pos.y + card_height);

        // --- 2. カード全体を覆う D&D / クリック用不可視ボタン ---
        ImGui::SetCursorScreenPos(start_pos);
        
        // 次に配置される ... ボタンにクリックイベントを通す設定
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("##card_selectable", ImVec2(avail_width, card_height));

        // --- Drag & Drop Source ---
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("STACK_CARD_MOVE", &i, sizeof(size_t));
            ImGui::Text("Moving: %s", title.c_str());
            ImGui::EndDragDropSource();
        }

        // --- Drag & Drop Target ---
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("STACK_CARD_MOVE")) {
                size_t from_i = *(const size_t*)payload->Data;
                state.reorder_item(from_i, i);
            }
            ImGui::EndDragDropTarget();
        }

        // --- 3. ... ボタン（右上） ---
        ImGui::SetCursorScreenPos(ImVec2(end_pos.x - 45.0f * base_scale, start_pos.y + 8.0f * base_scale));
        if (ImGui::Button("...", ImVec2(35.0f * base_scale, 0))) {
            ImGui::OpenPopup("CardMenu");
        }

        if (ImGui::BeginPopup("CardMenu")) {
            if (ImGui::MenuItem("Edit")) {
                editing_index = static_cast<int>(i);
                snprintf(edit_title, sizeof(edit_title), "%s", title.c_str());
                snprintf(edit_context, sizeof(edit_context), "%s", context.c_str());
                snprintf(edit_link, sizeof(edit_link), "%s", link.c_str());
            }
            if (ImGui::MenuItem("Delete")) {
                state.delete_item(i);
            }
            ImGui::EndPopup();
        }

        // 次のカードの位置へカーソルを更新
        ImGui::SetCursorScreenPos(ImVec2(start_pos.x, end_pos.y + 8.0f * base_scale));

        ImGui::PopID();
    }

    // 編集用モーダル
    if (editing_index >= 0) {
        ImGui::OpenPopup("Edit Card");
    }

    if (ImGui::BeginPopupModal("Edit Card", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Edit Title:");
        ImGui::InputText("##edit_title", edit_title, IM_ARRAYSIZE(edit_title));

        ImGui::Text("Edit Context:");
        ImGui::InputTextMultiline("##edit_context", edit_context, IM_ARRAYSIZE(edit_context), ImVec2(300 * base_scale, 100 * base_scale));

        ImGui::Text("Edit Link:");
        ImGui::InputText("##edit_link", edit_link, IM_ARRAYSIZE(edit_link));

        ImGui::Spacing();
        if (ImGui::Button("Save", ImVec2(120 * base_scale, 0))) {
            state.edit_item(editing_index, edit_title, edit_context, edit_link);
            editing_index = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120 * base_scale, 0))) {
            editing_index = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}