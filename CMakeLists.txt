cmake_minimum_required(VERSION 3.25)

project(TC
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(TC_Sandbox
    Sandbox/main.cpp
)

target_link_libraries(TC_Sandbox PRIVATE
    glfw
    glm::glm
    glad::glad
)

target_include_directories(TC_Sandbox PRIVATE
    Engine
)

if(WIN32)
    target_compile_definitions(TC_Sandbox PRIVATE
        GLFW_EXPOSE_NATIVE_WIN32
    )
endif()