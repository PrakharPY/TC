#include <iostream>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Platform/Window.h"
#include "Camera/Camera.h"
#include "Renderer/Renderer.h"

namespace
{
    TC::Camera* g_Camera = nullptr;

    bool g_FirstMouse = true;

    double g_LastMouseX = 0.0;
    double g_LastMouseY = 0.0;

    void MouseCallback(
        GLFWwindow* window,
        double xpos,
        double ypos)
    {
        if (!g_Camera)
            return;

        if (g_FirstMouse)
        {
            g_LastMouseX = xpos;
            g_LastMouseY = ypos;
            g_FirstMouse = false;
            return;
        }

        float xOffset =
            static_cast<float>(xpos - g_LastMouseX);

        float yOffset =
            static_cast<float>(g_LastMouseY - ypos);

        g_LastMouseX = xpos;
        g_LastMouseY = ypos;

        g_Camera->ProcessMouseMovement(
            xOffset,
            yOffset
        );
    }

    void ScrollCallback(
        GLFWwindow* window,
        double xOffset,
        double yOffset)
    {
        if (g_Camera)
        {
            g_Camera->ProcessMouseScroll(
                static_cast<float>(yOffset)
            );
        }
    }

    void ProcessInput(
        GLFWwindow* window,
        TC::Camera& camera,
        float deltaTime)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Forward, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Backward, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Left, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Right, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Up, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Down, deltaTime);
    }
}

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

    GLFWwindow* nativeWindow = window.GetNativeWindow();

    TC::Camera camera;

    g_Camera = &camera;

    TC::Renderer renderer;

    if (!renderer.Initialize())
    {
        g_Camera = nullptr;
        return -1;
    }

    // Capture mouse cursor
    glfwSetInputMode(
        nativeWindow,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    glfwSetCursorPosCallback(
        nativeWindow,
        MouseCallback
    );

    glfwSetScrollCallback(
        nativeWindow,
        ScrollCallback
    );

    std::cout << "\n";
    std::cout << "================================\n";
    std::cout << "       TC 0.4.1 - CAMERA\n";
    std::cout << "================================\n";
    std::cout << "WASD       - Move camera\n";
    std::cout << "SPACE      - Move upward\n";
    std::cout << "CTRL       - Move downward\n";
    std::cout << "MOUSE      - Look around\n";
    std::cout << "SCROLL     - Zoom\n";
    std::cout << "F          - Toggle wireframe\n";
    std::cout << "R          - Reset camera\n";
    std::cout << "ESC        - Exit\n";
    std::cout << "================================\n\n";

    double lastFrame = glfwGetTime();

    bool wireframe = false;
    bool previousF = false;
    bool previousR = false;

    while (!window.ShouldClose())
    {
        double currentFrame = glfwGetTime();

        float deltaTime =
            static_cast<float>(currentFrame - lastFrame);

        lastFrame = currentFrame;

        // Prevent unusually large movement after pauses
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ProcessInput(
            nativeWindow,
            camera,
            deltaTime
        );

        // Toggle wireframe
        bool currentF =
            glfwGetKey(nativeWindow, GLFW_KEY_F) == GLFW_PRESS;

        if (currentF && !previousF)
        {
            wireframe = !wireframe;
            renderer.SetWireframe(wireframe);

            std::cout << "TC: Wireframe "
                      << (wireframe ? "ON" : "OFF")
                      << "\n";
        }

        previousF = currentF;

        // Reset camera
        bool currentR =
            glfwGetKey(nativeWindow, GLFW_KEY_R) == GLFW_PRESS;

        if (currentR && !previousR)
        {
            camera.Reset();

            std::cout << "TC: Camera reset\n";
        }

        previousR = currentR;

        // Dynamic viewport and aspect ratio
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(
            nativeWindow,
            &width,
            &height
        );

        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);

            float aspectRatio =
                static_cast<float>(width) /
                static_cast<float>(height);

            renderer.Render(
                camera,
                aspectRatio
            );
        }

        window.SwapBuffers();
        window.PollEvents();
    }

    renderer.Shutdown();

    g_Camera = nullptr;

    std::cout << "TC: Application shutting down!\n";

    return 0;
}