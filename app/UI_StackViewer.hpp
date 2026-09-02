#pragma once
#include "AppState.hpp"

class UI_StackViewer {
private:
    int editing_index = -1;
    char edit_title[256] = "";
    char edit_context[2048] = "";
    char edit_link[512] = "";

public:
    void Render(AppState& state, float base_scale);
};