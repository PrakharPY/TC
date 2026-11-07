#include <iostream>

#include "Platform/Window.h"
#include "Renderer/Renderer.h"

int main()
{
    TC::Window window;

    if (!window.Create(
        1280,
        720,
        "TC - Lightweight 3D Development Platform"))
    {
        return -1;
    }

    TC::Renderer renderer;

    if (!renderer.Initialize())
    {
        return -1;
    }

    std::cout << "TC: Application running!\n";

    while (!window.ShouldClose())
    {
        renderer.Render();

        window.SwapBuffers();
        window.PollEvents();
    }

    renderer.Shutdown();

    std::cout << "TC: Application shutting down!\n";

    return 0;
}