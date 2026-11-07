#include <iostream>

// TC headers first (GLAD is included through Renderer.h)
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

// Prevent GLFW from including the OpenGL header
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int main()
{
    TC::Window window;

    if (!window.Create(
        1280,
        720,
        "TC - 3D Development Platform"))
    {
        return -1;
    }

    TC::Renderer renderer;

    if (!renderer.Initialize())
    {
        return -1;
    }

    std::cout << "TC: 3D application running!\n";

    while (!window.ShouldClose())
    {
        float currentTime =
            static_cast<float>(glfwGetTime());

        renderer.Render(currentTime);

        window.SwapBuffers();
        window.PollEvents();
    }

    renderer.Shutdown();

    std::cout << "TC: Application shutting down!\n";

    return 0;
}