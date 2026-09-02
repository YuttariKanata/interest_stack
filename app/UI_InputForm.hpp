#pragma once
#include "AppState.hpp"

class UI_InputForm {
private:
    char buf_title[256] = "";
    char buf_context[2048] = "";
    char buf_link[512] = "";

public:
    void Render(AppState& state, float base_scale);
};