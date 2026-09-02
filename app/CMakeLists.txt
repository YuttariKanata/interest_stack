cmake_minimum_required(VERSION 3.14)
project(interest_stack_pusher CXX)

set(CMAKE_CXX_STANDARD 17)

# GLFW & OpenGL
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ImGui 側のソースコード指定（事前にimguiリポジトリから持ってきている前提）
set(IMGUI_DIR ./imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(istack_push main.cpp ${IMGUI_SOURCES})

target_include_directories(istack_push PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(istack_push PRIVATE
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)