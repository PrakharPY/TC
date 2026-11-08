#include <iostream>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Platform/Window.h"
#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "Renderer/Renderer.h"

namespace
{
    TC::Camera* g_Camera = nullptr;

    bool g_FirstMouse = true;

    float g_LastX = 640.0f;
    float g_LastY = 360.0f;

    void MouseCallback(
        GLFWwindow* window,
        double xpos,
        double ypos
    )
    {
        if (!g_Camera)
            return;

        if (g_FirstMouse)
        {
            g_LastX = static_cast<float>(xpos);
            g_LastY = static_cast<float>(ypos);
            g_FirstMouse = false;
        }

        float xOffset = static_cast<float>(xpos) - g_LastX;
        float yOffset = g_LastY - static_cast<float>(ypos);

        g_LastX = static_cast<float>(xpos);
        g_LastY = static_cast<float>(ypos);

        g_Camera->ProcessMouseMovement(xOffset, yOffset);
    }

    void ScrollCallback(
        GLFWwindow* window,
        double xOffset,
        double yOffset
    )
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
        float deltaTime
    )
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

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

    if (!window.Create(1280, 720, "TC - 3D Development Platform"))
        return -1;

    GLFWwindow* nativeWindow = window.GetNativeWindow();

    TC::Camera camera;
    g_Camera = &camera;

    TC::Scene scene;

    TC::Renderer renderer;

    if (!renderer.Initialize())
    {
        std::cerr << "Renderer initialization failed!\n";
        return -1;
    }

    glfwSetInputMode(
        nativeWindow,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    glfwSetCursorPosCallback(nativeWindow, MouseCallback);
    glfwSetScrollCallback(nativeWindow, ScrollCallback);

    std::cout << "TC Milestone 0.5\n";
    std::cout << "WASD: Move | Mouse: Look | Scroll: Zoom\n";
    std::cout << "Space/Ctrl: Up/Down | F: Wireframe | R: Reset\n";

    float lastFrame = 0.0f;

    bool wireframeKeyWasPressed = false;
    bool resetKeyWasPressed = false;

    while (!glfwWindowShouldClose(nativeWindow))
    {
        float currentFrame = static_cast<float>(glfwGetTime());

        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ProcessInput(nativeWindow, camera, deltaTime);

        bool wireframePressed =
            glfwGetKey(nativeWindow, GLFW_KEY_F) == GLFW_PRESS;

        if (wireframePressed && !wireframeKeyWasPressed)
        {
            static bool wireframe = false;
            wireframe = !wireframe;

            renderer.SetWireframe(wireframe);

            std::cout << "TC: Wireframe "
                      << (wireframe ? "ON" : "OFF")
                      << '\n';
        }

        wireframeKeyWasPressed = wireframePressed;

        bool resetPressed =
            glfwGetKey(nativeWindow, GLFW_KEY_R) == GLFW_PRESS;

        if (resetPressed && !resetKeyWasPressed)
        {
            camera.Reset();
            g_FirstMouse = true;

            std::cout << "TC: Camera reset\n";
        }

        resetKeyWasPressed = resetPressed;

        int width, height;

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

            renderer.Render(camera, scene, aspectRatio);
        }

        glfwSwapBuffers(nativeWindow);
        glfwPollEvents();
    }

    renderer.Shutdown();

    g_Camera = nullptr;

    std::cout << "TC: Application shutting down!\n";

    return 0;
}