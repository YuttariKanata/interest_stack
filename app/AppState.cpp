#include "AppState.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h> // Win32 API

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

AppState::AppState() {
    load_all();
}

void AppState::load_all() {
    std::ifstream cfg_file(config_path);
    if (cfg_file.is_open()) {
        try {
            json j;
            cfg_file >> j;
            if (j.contains("font_scale")) config.font_scale = j["font_scale"].get<float>();
            if (j.contains("is_dark_mode")) config.is_dark_mode = j["is_dark_mode"].get<bool>();
        } catch (...) {}
    }

    std::ifstream ifs(json_path);
    if (ifs.is_open()) {
        try {
            ifs >> stack_data;
        } catch (...) {
            stack_data = json::object();
            stack_data["stack"] = json::array();
        }
    } else {
        stack_data = json::object();
        stack_data["stack"] = json::array();
    }
    is_dirty = false;
    is_saving = false;
}

void AppState::save_config() {
    json j;
    j["font_scale"] = config.font_scale;
    j["is_dark_mode"] = config.is_dark_mode;
    std::ofstream ofs(config_path);
    if (ofs.is_open()) {
        ofs << j.dump(4) << std::endl;
    }
}

void AppState::push_undo_state() {
    undo_stack.push_back(stack_data["stack"]);
    redo_stack.clear();
    is_dirty = true;
}

bool AppState::undo() {
    if (undo_stack.empty() || is_saving) return false;
    
    redo_stack.push_back(stack_data["stack"]);
    stack_data["stack"] = undo_stack.back();
    undo_stack.pop_back();
    is_dirty = true;
    return true;
}

bool AppState::redo() {
    if (redo_stack.empty() || is_saving) return false;

    undo_stack.push_back(stack_data["stack"]);
    stack_data["stack"] = redo_stack.back();
    redo_stack.pop_back();
    is_dirty = true;
    return true;
}

// Windowsでウィンドウを出さずにコマンドを実行するヘルパー
static int execute_hidden_cmd(const std::string& cmd_str) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // 画面を非表示にする

    ZeroMemory(&pi, sizeof(pi));

    // CreateProcessA は第一引数か第二引数のバッファを書き換える可能性があるため char 配列にする
    std::vector<char> cmd_buf(cmd_str.begin(), cmd_str.end());
    cmd_buf.push_back('\0');

    if (!CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exit_code);
}

void AppState::start_save_and_git(const std::string& commit_title) {
    if (is_saving) return;

    is_saving = true;
    status_msg = "Saving & Committing...";
    stack_data["updated_at"] = get_current_time_iso();

    json data_to_save = stack_data;
    std::string path_to_json = json_path;

    save_future = std::async(std::launch::async, [data_to_save, path_to_json, commit_title]() -> std::pair<bool, std::string> {
        std::ofstream ofs(path_to_json);
        if (!ofs.is_open()) {
            return {false, "Error: Failed to open stack.json."};
        }
        ofs << data_to_save.dump(4) << std::endl;
        ofs.close();

        std::ofstream msg_file("commit_msg.txt");
        msg_file << commit_title << std::endl;
        msg_file.close();

        std::string git_cmd = "cmd.exe /c \"cd /d .. && git add stack.json && git commit -F app/commit_msg.txt && git push\" > git_output.log 2>&1";
        int res = execute_hidden_cmd(git_cmd);
        std::remove("commit_msg.txt");

        if (res != 0) {
            std::ifstream log_file("git_output.log");
            std::string log_content((std::istreambuf_iterator<char>(log_file)), std::istreambuf_iterator<char>());
            return {false, log_content.empty() ? ("Git error code: " + std::to_string(res)) : ("Git error: " + log_content)};
        }

        return {true, "Saved & Pushed successfully."};
    });
}

void AppState::update_async_save() {
    if (!is_saving) return;

    if (save_future.valid() && save_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        auto result = save_future.get();
        bool success = result.first;
        status_msg = result.second;

        if (success) {
            is_dirty = false;
        }
        is_saving = false;
    }
}

void AppState::push_item(const std::string& title, const std::string& context, const std::string& link) {
    push_undo_state();

    json new_item;
    new_item["id"] = "frame-" + std::to_string(stack_data["stack"].size());
    new_item["title"] = title;
    new_item["context"] = context;
    new_item["link"] = link;
    new_item["pushed_at"] = get_current_date();

    stack_data["stack"].insert(stack_data["stack"].begin(), new_item);
}

void AppState::delete_item(size_t index) {
    if (index >= stack_data["stack"].size()) return;
    push_undo_state();
    stack_data["stack"].erase(stack_data["stack"].begin() + index);
}

void AppState::edit_item(size_t index, const std::string& title, const std::string& context, const std::string& link) {
    if (index >= stack_data["stack"].size()) return;
    push_undo_state();

    stack_data["stack"][index]["title"] = title;
    stack_data["stack"][index]["context"] = context;
    stack_data["stack"][index]["link"] = link;
}

void AppState::reorder_item(size_t from_index, size_t to_index) {
    if (from_index >= stack_data["stack"].size() || to_index >= stack_data["stack"].size()) return;
    if (from_index == to_index) return;

    push_undo_state();

    auto item = stack_data["stack"][from_index];
    stack_data["stack"].erase(stack_data["stack"].begin() + from_index);
    stack_data["stack"].insert(stack_data["stack"].begin() + to_index, item);
}