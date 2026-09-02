cmake_minimum_required(VERSION 3.10)
project(interest_stack)

set(CMAKE_CXX_STANDARD 17)

# --- IME 有効化マクロを定義 ---
# プロジェクト全体（ImGuiのソースコードを含む）に定義を伝播させる
add_compile_definitions(IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS)

# ソースファイルの指定
add_executable(istack_push
    main.cpp
    # もし ImGui や GLFW バックエンドを直接コンパイル対象に含めている場合はここに追加されています
    # imgui/imgui.cpp
    # imgui/imgui_draw.cpp
    # imgui/imgui_tables.cpp
    # imgui/imgui_widgets.cpp
    # imgui/backends/imgui_impl_glfw.cpp
    # imgui/backends/imgui_impl_opengl3.cpp
)

# リンクライブラリに imm32 を追加
# (Win32 APIの IME 制御モジュール ImmSetCompositionWindow 等のために必須)
target_link_libraries(istack_push PRIVATE
    glfw
    opengl32
    imm32.lib
)