#include <iostream>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Platform/Window.h"
#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "Renderer/Renderer.h"

namespace
{
    TC::Camera* g_Camera = nullptr;
    TC::Scene* g_Scene = nullptr;

    int g_SelectedObject = -1;

    bool g_FirstMouse = true;
    bool g_RightMouseDown = false;

    float g_LastX = 640.0f;
    float g_LastY = 360.0f;

    bool g_Wireframe = false;
    bool g_WireframeKeyWasPressed = false;
    bool g_ResetKeyWasPressed = false;

    glm::vec3 ScreenPointToRay(
        float mouseX,
        float mouseY,
        int windowWidth,
        int windowHeight,
        const TC::Camera& camera
    )
    {
        float x = (2.0f * mouseX) / windowWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / windowHeight;

        glm::vec4 rayClip(x, y, -1.0f, 1.0f);

        glm::mat4 projection =
            camera.GetProjectionMatrix(
                static_cast<float>(windowWidth) /
                static_cast<float>(windowHeight)
            );

        glm::vec4 rayEye =
            glm::inverse(projection) * rayClip;

        rayEye.z = -1.0f;
        rayEye.w = 0.0f;

        glm::vec3 rayWorld =
            glm::vec3(
                glm::inverse(camera.GetViewMatrix()) * rayEye
            );

        return glm::normalize(rayWorld);
    }

    void MouseCallback(
        GLFWwindow* window,
        double xpos,
        double ypos
    )
    {
        if (!g_Camera || !g_RightMouseDown)
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

    void MouseButtonCallback(
        GLFWwindow* window,
        int button,
        int action,
        int mods
    )
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
            {
                g_RightMouseDown = true;
                g_FirstMouse = true;

                glfwSetInputMode(
                    window,
                    GLFW_CURSOR,
                    GLFW_CURSOR_DISABLED
                );
            }
            else if (action == GLFW_RELEASE)
            {
                g_RightMouseDown = false;

                glfwSetInputMode(
                    window,
                    GLFW_CURSOR,
                    GLFW_CURSOR_NORMAL
                );
            }
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT &&
            action == GLFW_PRESS &&
            !g_RightMouseDown &&
            g_Camera &&
            g_Scene)
        {
            double mouseX, mouseY;

            glfwGetCursorPos(window, &mouseX, &mouseY);

            int width, height;

            glfwGetWindowSize(window, &width, &height);

            if (width <= 0 || height <= 0)
                return;

            glm::vec3 rayDirection = ScreenPointToRay(
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                width,
                height,
                *g_Camera
            );

            g_SelectedObject =
                g_Scene->PickObject(
                    g_Camera->GetPosition(),
                    rayDirection
                );

            if (g_SelectedObject >= 0)
            {
                std::cout << "TC: Selected object "
                          << g_SelectedObject << '\n';
            }
            else
            {
                std::cout << "TC: Selection cleared\n";
            }
        }
    }

    void ProcessCameraInput(
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

    void ProcessObjectTransform(
        GLFWwindow* window,
        TC::Scene& scene,
        int selectedObject,
        float deltaTime
    )
    {
        TC::SceneObject* object = scene.GetObject(selectedObject);

        if (!object)
            return;

        // Translation speed.
        float moveSpeed = 2.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            object->Translate(glm::vec3(-moveSpeed, 0.0f, 0.0f));

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            object->Translate(glm::vec3(moveSpeed, 0.0f, 0.0f));

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            object->Translate(glm::vec3(0.0f, 0.0f, -moveSpeed));

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            object->Translate(glm::vec3(0.0f, 0.0f, moveSpeed));

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            object->Translate(glm::vec3(0.0f, moveSpeed, 0.0f));

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            object->Translate(glm::vec3(0.0f, -moveSpeed, 0.0f));

        // Rotation speed.
        float rotationSpeed = 90.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            object->Rotate(glm::vec3(0.0f, -rotationSpeed, 0.0f));

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            object->Rotate(glm::vec3(0.0f, rotationSpeed, 0.0f));

        // Scaling.
        float scaleSpeed = 1.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
            object->ScaleBy(1.0f - scaleSpeed);

        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
            object->ScaleBy(1.0f + scaleSpeed);
    }
}

int main()
{
    TC::Window window;

    if (!window.Create(1280, 720, "TC - 3D Development Platform"))
        return -1;

    GLFWwindow* nativeWindow = window.GetNativeWindow();

    TC::Camera camera;
    TC::Scene scene;
    TC::Renderer renderer;

    g_Camera = &camera;
    g_Scene = &scene;

    if (!renderer.Initialize())
    {
        std::cerr << "Renderer initialization failed!\n";
        return -1;
    }

    glfwSetInputMode(
        nativeWindow,
        GLFW_CURSOR,
        GLFW_CURSOR_NORMAL
    );

    glfwSetCursorPosCallback(nativeWindow, MouseCallback);
    glfwSetScrollCallback(nativeWindow, ScrollCallback);
    glfwSetMouseButtonCallback(nativeWindow, MouseButtonCallback);

    std::cout << "\nTC Milestone 0.8 - Object Transformation\n\n";

    std::cout << "SELECTION:\n";
    std::cout << "Left Click: Select object\n";
    std::cout << "Right Mouse: Hold to look around\n\n";

    std::cout << "TRANSLATION:\n";
    std::cout << "Arrow Keys: Move X/Z\n";
    std::cout << "Page Up/Down: Move Y\n\n";

    std::cout << "ROTATION:\n";
    std::cout << "Q/E: Rotate around Y\n\n";

    std::cout << "SCALING:\n";
    std::cout << "-/+: Scale down/up\n\n";

    std::cout << "CAMERA:\n";
    std::cout << "WASD: Move camera\n";
    std::cout << "Space/Ctrl: Up/Down\n";
    std::cout << "Scroll: Zoom\n";
    std::cout << "F: Wireframe | R: Reset camera\n\n";

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(nativeWindow))
    {
        float currentFrame = static_cast<float>(glfwGetTime());

        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ProcessCameraInput(nativeWindow, camera, deltaTime);

        ProcessObjectTransform(
            nativeWindow,
            scene,
            g_SelectedObject,
            deltaTime
        );

        // Wireframe toggle.
        bool wireframePressed =
            glfwGetKey(nativeWindow, GLFW_KEY_F) == GLFW_PRESS;

        if (wireframePressed && !g_WireframeKeyWasPressed)
        {
            g_Wireframe = !g_Wireframe;
            renderer.SetWireframe(g_Wireframe);

            std::cout << "TC: Wireframe "
                      << (g_Wireframe ? "ON" : "OFF")
                      << '\n';
        }

        g_WireframeKeyWasPressed = wireframePressed;

        // Camera reset.
        bool resetPressed =
            glfwGetKey(nativeWindow, GLFW_KEY_R) == GLFW_PRESS;

        if (resetPressed && !g_ResetKeyWasPressed)
        {
            camera.Reset();
            g_FirstMouse = true;

            std::cout << "TC: Camera reset\n";
        }

        g_ResetKeyWasPressed = resetPressed;

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

            renderer.Render(
                camera,
                scene,
                aspectRatio,
                g_SelectedObject
            );
        }

        glfwSwapBuffers(nativeWindow);
        glfwPollEvents();
    }

    renderer.Shutdown();

    g_Camera = nullptr;
    g_Scene = nullptr;

    std::cout << "TC: Application shutting down!\n";

    return 0;
}