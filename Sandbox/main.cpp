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
#include "Editor/TransformGizmo.h"

namespace
{
    TC::Camera* g_Camera = nullptr;
    TC::Scene* g_Scene = nullptr;
    TC::TransformGizmo* g_Gizmo = nullptr;

    int g_SelectedObject = -1;

    bool g_FirstMouse = true;
    bool g_RightMouseDown = false;

    float g_LastX = 640.0f;
    float g_LastY = 360.0f;

    bool g_Wireframe = false;
    bool g_WireframeKeyWasPressed = false;
    bool g_ResetKeyWasPressed = false;
    bool g_TranslateKeyWasPressed = false;
    bool g_RotateKeyWasPressed = false;

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

    void UpdateGizmoHover(GLFWwindow* window)
    {
        if (!g_Camera || !g_Scene || !g_Gizmo ||
            g_SelectedObject < 0 ||
            g_RightMouseDown ||
            g_Gizmo->IsDragging())
        {
            return;
        }

        TC::SceneObject* selected =
            g_Scene->GetObject(g_SelectedObject);

        if (!selected)
            return;

        double mouseX, mouseY;

        glfwGetCursorPos(window, &mouseX, &mouseY);

        int width, height;

        glfwGetWindowSize(window, &width, &height);

        if (width <= 0 || height <= 0)
            return;

        g_Gizmo->UpdateHover(
            *g_Camera,
            selected->GetPosition(),
            static_cast<float>(mouseX),
            static_cast<float>(mouseY),
            width,
            height
        );
    }

    void MouseCallback(
        GLFWwindow* window,
        double xpos,
        double ypos
    )
    {
        if (g_RightMouseDown && g_Camera)
        {
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
            return;
        }

        UpdateGizmoHover(window);
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
            }
            else if (action == GLFW_RELEASE)
            {
                g_RightMouseDown = false;
                UpdateGizmoHover(window);
            }
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT &&
            action == GLFW_RELEASE)
        {
            if (g_Gizmo && g_Gizmo->IsDragging())
            {
                g_Gizmo->EndDrag();
                UpdateGizmoHover(window);
            }

            return;
        }

        if (button != GLFW_MOUSE_BUTTON_LEFT ||
            action != GLFW_PRESS ||
            g_RightMouseDown ||
            !g_Camera ||
            !g_Scene)
        {
            return;
        }

        double mouseX, mouseY;

        glfwGetCursorPos(window, &mouseX, &mouseY);

        int width, height;

        glfwGetWindowSize(window, &width, &height);

        if (width <= 0 || height <= 0)
            return;

        if (g_SelectedObject >= 0 && g_Gizmo)
        {
            TC::SceneObject* selected =
                g_Scene->GetObject(g_SelectedObject);

            if (selected)
            {
                if (g_Gizmo->BeginDrag(
                    *g_Camera,
                    selected->GetPosition(),
                    static_cast<float>(mouseX),
                    static_cast<float>(mouseY),
                    width,
                    height))
                {
                    TC::GizmoAxis axis =
                        g_Gizmo->GetActiveAxis();

                    std::cout << "TC: Dragging axis: ";

                    switch (axis)
                    {
                    case TC::GizmoAxis::X:
                        std::cout << "X\n";
                        break;

                    case TC::GizmoAxis::Y:
                        std::cout << "Y\n";
                        break;

                    case TC::GizmoAxis::Z:
                        std::cout << "Z\n";
                        break;

                    default:
                        break;
                    }

                    return;
                }
            }
        }

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

        if (g_Gizmo)
            g_Gizmo->ClearActiveAxis();

        if (g_SelectedObject >= 0)
        {
            std::cout << "TC: Selected object "
                      << g_SelectedObject << '\n';
        }
        else
        {
            std::cout << "TC: Selection cleared\n";
        }

        UpdateGizmoHover(window);
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

        // C replaces Ctrl for camera-down movement.
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
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

        float rotationSpeed = 90.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            object->Rotate(glm::vec3(0.0f, -rotationSpeed, 0.0f));

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            object->Rotate(glm::vec3(0.0f, rotationSpeed, 0.0f));

        float scaleSpeed = 1.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
            object->ScaleBy(1.0f - scaleSpeed);

        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
            object->ScaleBy(1.0f + scaleSpeed);
    }

    void ProcessGizmoDragging(
        GLFWwindow* window,
        TC::Scene& scene,
        TC::TransformGizmo& gizmo,
        int selectedObject
    )
    {
        if (!gizmo.IsDragging())
            return;

        TC::SceneObject* object =
            scene.GetObject(selectedObject);

        if (!object)
        {
            gizmo.EndDrag();
            return;
        }

        double mouseX, mouseY;

        glfwGetCursorPos(window, &mouseX, &mouseY);

        int width, height;

        glfwGetWindowSize(window, &width, &height);

        if (width <= 0 || height <= 0)
            return;

        // Ctrl enables grid snapping while dragging.
        bool snapping =
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        if (gizmo.GetMode() == TC::GizmoMode::Translate)
        {
            glm::vec3 movement = gizmo.UpdateDrag(
                *g_Camera,
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                width,
                height,
                snapping
            );
            object->Translate(movement);
        }
        else
        {
            float rotationDelta = gizmo.UpdateRotationDrag(
                *g_Camera,
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                width,
                height,
                snapping
            );

            glm::vec3 axis(0.0f);
            switch (gizmo.GetActiveAxis())
            {
            case TC::GizmoAxis::X: axis.x = 1.0f; break;
            case TC::GizmoAxis::Y: axis.y = 1.0f; break;
            case TC::GizmoAxis::Z: axis.z = 1.0f; break;
            default: break;
            }
            object->Rotate(axis * rotationDelta);
        }
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
    TC::TransformGizmo gizmo;

    g_Camera = &camera;
    g_Scene = &scene;
    g_Gizmo = &gizmo;

    if (!renderer.Initialize())
        return -1;

    if (!gizmo.Initialize())
    {
        renderer.Shutdown();
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

    std::cout << "\nTC Milestone 0.9.5 - Rotation Gizmo\n\n";

    std::cout << "Left Click: Select object / Grab gizmo axis\n";
    std::cout << "G: Translation gizmo | T: Rotation gizmo\n";
    std::cout << "Left Mouse Drag: Transform along selected axis/ring\n";
    std::cout << "Hold Ctrl: Snap translation to 0.5 units / rotation to 15 degrees\n";
    std::cout << "Right Mouse: Hold to look around\n";
    std::cout << "WASD: Move camera | Scroll: Zoom\n";
    std::cout << "Space/C: Move camera up/down\n";
    std::cout << "Arrow Keys: Move object X/Z\n";
    std::cout << "Page Up/Down: Move object Y\n";
    std::cout << "Q/E: Rotate | -/+: Scale\n";
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

        bool translatePressed = glfwGetKey(nativeWindow, GLFW_KEY_G) == GLFW_PRESS;
        if (translatePressed && !g_TranslateKeyWasPressed && !gizmo.IsDragging())
        {
            gizmo.SetMode(TC::GizmoMode::Translate);
            std::cout << "TC: Translation gizmo selected\n";
        }
        g_TranslateKeyWasPressed = translatePressed;

        bool rotatePressed = glfwGetKey(nativeWindow, GLFW_KEY_T) == GLFW_PRESS;
        if (rotatePressed && !g_RotateKeyWasPressed && !gizmo.IsDragging())
        {
            gizmo.SetMode(TC::GizmoMode::Rotate);
            std::cout << "TC: Rotation gizmo selected\n";
        }
        g_RotateKeyWasPressed = rotatePressed;

        if (gizmo.IsDragging())
        {
            ProcessGizmoDragging(
                nativeWindow,
                scene,
                gizmo,
                g_SelectedObject
            );
        }
        else
        {
            ProcessObjectTransform(
                nativeWindow,
                scene,
                g_SelectedObject,
                deltaTime
            );
        }

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

        bool resetPressed =
            glfwGetKey(nativeWindow, GLFW_KEY_R) == GLFW_PRESS;

        if (resetPressed && !g_ResetKeyWasPressed)
        {
            camera.Reset();
            g_FirstMouse = true;
        }

        g_ResetKeyWasPressed = resetPressed;

        int width, height;

        glfwGetFramebufferSize(nativeWindow, &width, &height);

        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);

            float aspectRatio =
                static_cast<float>(width) /
                static_cast<float>(height);

            TC::SceneObject* selected =
                scene.GetObject(g_SelectedObject);

            if (selected && !g_RightMouseDown &&
                !gizmo.IsDragging())
            {
                UpdateGizmoHover(nativeWindow);
            }

            renderer.Render(
                camera,
                scene,
                aspectRatio,
                g_SelectedObject
            );

            if (selected &&
                selected->GetType() == TC::PrimitiveType::Cube)
            {
                gizmo.Render(
                    camera,
                    selected->GetPosition(),
                    aspectRatio
                );
            }
        }

        glfwSwapBuffers(nativeWindow);
        glfwPollEvents();
    }

    gizmo.EndDrag();
    gizmo.Shutdown();
    renderer.Shutdown();

    g_Camera = nullptr;
    g_Scene = nullptr;
    g_Gizmo = nullptr;

    std::cout << "TC: Application shutting down!\n";

    return 0;
}