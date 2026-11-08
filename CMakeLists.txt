cmake_minimum_required(VERSION 3.25)

project(TC VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_library(TC STATIC

    Engine/Platform/Window.cpp

    Engine/Camera/Camera.cpp

    Engine/Scene/SceneObject.cpp
    Engine/Scene/Scene.cpp

    Engine/Renderer/Shader.cpp
    Engine/Renderer/Renderer.cpp
)

target_include_directories(TC PUBLIC Engine)

target_link_libraries(TC PUBLIC
    glfw
    glm::glm
    glad::glad
)

add_executable(TC_Sandbox
    Sandbox/main.cpp
)

target_link_libraries(TC_Sandbox PRIVATE TC)

set_property(
    TARGET TC_Sandbox
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)