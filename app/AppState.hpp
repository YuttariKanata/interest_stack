#pragma once

#include <string>
#include <vector>
#include <future>
#include "single_include/json.hpp"

using json = nlohmann::json;

struct AppConfig {
    float font_scale = 1.0f;
    bool is_dark_mode = true;
};

class AppState {
public:
    json stack_data;
    AppConfig config;
    std::string status_msg;
    bool is_dirty = false;
    bool is_saving = false; // 保存・Git処理中フラグ

    // メモリ内 Undo / Redo（オブジェクト全体を保持して updated_at も巻き戻せるように保持）
    std::vector<json> undo_stack;
    std::vector<json> redo_stack;

    const std::string json_path = "../stack.json";
    const std::string config_path = "config.json";

    AppState();

    void load_all();
    void save_config();
    
    // 非同期保存の開始と状態更新チェック
    void start_save_and_git(const std::string& commit_title = "Update stack");
    void update_async_save();

    void push_undo_state();
    bool undo();
    bool redo();

    void touch_updated_at(); // updated_at を現在時刻(UTC)に更新

    void push_item(const std::string& title, const std::string& context, const std::string& link);
    void delete_item(size_t index);
    void edit_item(size_t index, const std::string& title, const std::string& context, const std::string& link);
    void reorder_item(size_t from_index, size_t to_index);

private:
    std::future<std::pair<bool, std::string>> save_future;
};

std::string get_current_time_iso();
std::string get_current_date();