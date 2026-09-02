cmake_minimum_required(VERSION 3.10)
project(interest_stack)

set(CMAKE_CXX_STANDARD 17)

# --- IME 有効化マクロを全体に定義 ---
add_compile_definitions(IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS)

# インクルードディレクトリの追加（カレントディレクトリと ImGui のパス）
include_directories(. imgui imgui/backends)

# ソースファイルの指定（新規作成したファイルをすべて追加）
add_executable(istack_push
    main.cpp
    AppState.cpp
    UI_InputForm.cpp
    UI_StackViewer.cpp
    # もし ImGui や GLFW バックエンドの .cpp を直接コンパイルしている場合はここにも記述
    # imgui/imgui.cpp
    # imgui/imgui_draw.cpp
    # imgui/imgui_tables.cpp
    # imgui/imgui_widgets.cpp
    # imgui/backends/imgui_impl_glfw.cpp
    # imgui/backends/imgui_impl_opengl3.cpp
)

# リンクライブラリの指定 (Win32 IME 用の imm32 も追加)
target_link_libraries(istack_push PRIVATE
    glfw
    opengl32
    imm32
)