#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <cmath>

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
#include "Editor/FileBrowser.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace
{
    TC::Camera* g_Camera = nullptr;
    TC::Scene* g_Scene = nullptr;
    TC::TransformGizmo* g_Gizmo = nullptr;
    TC::FileBrowser g_FileBrowser;

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
    bool g_ScaleKeyWasPressed = false;
    bool g_DuplicateKeyWasPressed = false;
    bool g_DeleteKeyWasPressed = false;
    bool g_CreateCubeKeyWasPressed = false;
    bool g_CreatePlaneKeyWasPressed = false;
    bool g_EscapeKeyWasPressed = false;
    bool g_UndoKeyWasPressed = false;
    bool g_RedoKeyWasPressed = false;
    bool g_SaveKeyWasPressed = false;
    bool g_LoadKeyWasPressed = false;
    bool g_SaveAsKeyWasPressed = false;

    const std::string DEFAULT_SCENE_PATH = "Assets/Scenes/Untitled.tcscene";
    std::string g_CurrentScenePath = DEFAULT_SCENE_PATH;

    bool SaveSceneToFile(const TC::Scene& scene, const std::string& path)
    {
        try
        {
            const std::filesystem::path filePath(path);
            if (filePath.has_parent_path())
                std::filesystem::create_directories(filePath.parent_path());

            std::ofstream file(path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
                return false;

            const auto& objects = scene.GetObjects();
            file << "TC_SCENE 1\n";
            file << objects.size() << '\n';
            file << std::setprecision(9);

            for (const TC::SceneObject& object : objects)
            {
                const glm::vec3 position = object.GetPosition();
                const glm::vec3 rotation = object.GetRotation();
                const glm::vec3 scale = object.GetScale();
                const glm::vec3 color = object.GetColor();

                file << static_cast<int>(object.GetType()) << ' '
                     << position.x << ' ' << position.y << ' ' << position.z << ' '
                     << rotation.x << ' ' << rotation.y << ' ' << rotation.z << ' '
                     << scale.x << ' ' << scale.y << ' ' << scale.z << ' '
                     << color.x << ' ' << color.y << ' ' << color.z << '\n';
            }

            return file.good();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool LoadSceneFromFile(TC::Scene& scene, const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return false;

        std::string magic;
        int version = 0;
        std::size_t objectCount = 0;
        if (!(file >> magic >> version) || magic != "TC_SCENE" || version != 1 ||
            !(file >> objectCount) || objectCount == 0 || objectCount > 100000)
            return false;

        std::vector<TC::SceneObject> objects;
        objects.reserve(objectCount);

        for (std::size_t i = 0; i < objectCount; ++i)
        {
            int typeValue = -1;
            glm::vec3 position, rotation, scale, color;
            if (!(file >> typeValue
                >> position.x >> position.y >> position.z
                >> rotation.x >> rotation.y >> rotation.z
                >> scale.x >> scale.y >> scale.z
                >> color.x >> color.y >> color.z))
                return false;

            if (typeValue < static_cast<int>(TC::PrimitiveType::Cube) ||
                typeValue > static_cast<int>(TC::PrimitiveType::Plane))
                return false;

            const glm::vec3 values[] = { position, rotation, scale, color };
            for (const glm::vec3& value : values)
            {
                if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
                    !std::isfinite(value.z))
                    return false;
            }

            if (scale.x <= 0.0f || scale.y <= 0.0f || scale.z <= 0.0f)
                return false;

            objects.emplace_back(
                static_cast<TC::PrimitiveType>(typeValue),
                position, rotation, scale, color);
        }

        // Do not replace the live scene unless the entire file was parsed successfully.
        scene.SetObjects(objects);
        return true;
    }

    class SceneHistory
    {
    public:
        void Initialize(const TC::Scene& scene, int selectedObject)
        {
            m_States.clear();
            m_CurrentState = 0;
            m_HasPendingState = false;
            m_States.push_back({ scene.GetObjects(), selectedObject });
        }

        void Observe(const TC::Scene& scene, int selectedObject, double currentTime)
        {
            State current{ scene.GetObjects(), selectedObject };
            const State& baseline = m_HasPendingState
                ? m_PendingState
                : m_States[m_CurrentState];

            if (!ObjectsEqual(current.objects, baseline.objects))
            {
                m_PendingState = current;
                m_HasPendingState = true;
                m_LastChangeTime = currentTime;
                return;
            }

            if (m_HasPendingState && currentTime - m_LastChangeTime >= HISTORY_IDLE_SECONDS)
                CommitPending();
        }

        bool Undo(TC::Scene& scene, int& selectedObject)
        {
            CommitPending();
            if (m_CurrentState == 0)
                return false;

            --m_CurrentState;
            Restore(scene, selectedObject, m_States[m_CurrentState]);
            return true;
        }

        bool Redo(TC::Scene& scene, int& selectedObject)
        {
            CommitPending();
            if (m_CurrentState + 1 >= m_States.size())
                return false;

            ++m_CurrentState;
            Restore(scene, selectedObject, m_States[m_CurrentState]);
            return true;
        }

    private:
        struct State
        {
            std::vector<TC::SceneObject> objects;
            int selectedObject = -1;
        };

        static constexpr double HISTORY_IDLE_SECONDS = 0.4;

        static bool VectorEqual(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec3(0.000001f)));
        }

        static bool ObjectsEqual(
            const std::vector<TC::SceneObject>& a,
            const std::vector<TC::SceneObject>& b)
        {
            if (a.size() != b.size())
                return false;

            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (a[i].GetType() != b[i].GetType() ||
                    !VectorEqual(a[i].GetPosition(), b[i].GetPosition()) ||
                    !VectorEqual(a[i].GetRotation(), b[i].GetRotation()) ||
                    !VectorEqual(a[i].GetScale(), b[i].GetScale()) ||
                    !VectorEqual(a[i].GetColor(), b[i].GetColor()))
                    return false;
            }

            return true;
        }

        void CommitPending()
        {
            if (!m_HasPendingState)
                return;

            // A new edit after Undo discards the old redo branch.
            m_States.resize(m_CurrentState + 1);
            m_States.push_back(m_PendingState);
            m_CurrentState = m_States.size() - 1;
            m_HasPendingState = false;
        }

        static void Restore(TC::Scene& scene, int& selectedObject, const State& state)
        {
            scene.SetObjects(state.objects);
            selectedObject = state.selectedObject;
        }

        std::vector<State> m_States;
        std::size_t m_CurrentState = 0;
        State m_PendingState;
        bool m_HasPendingState = false;
        double m_LastChangeTime = 0.0;
    };

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
        if (g_FileBrowser.IsOpen())
            return;

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
        if (g_FileBrowser.IsOpen())
            return;

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
        if (g_FileBrowser.IsOpen())
            return;

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

                    case TC::GizmoAxis::Uniform:
                        std::cout << "Uniform\n";
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
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Forward, deltaTime);

        // S moves the camera backward only when Ctrl is not held.
        // This prevents Ctrl+S (Save Scene) from also moving the camera.
        const bool controlDown =
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !controlDown)
            camera.ProcessKeyboard(TC::CameraMovement::Backward, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(TC::CameraMovement::Left, deltaTime);

        bool shiftDown =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !shiftDown)
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
        else if (gizmo.GetMode() == TC::GizmoMode::Rotate)
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
        else
        {
            float scaleDelta = gizmo.UpdateScaleDrag(
                *g_Camera,
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                width,
                height,
                snapping
            );

            glm::vec3 newScale = object->GetScale();
            if (gizmo.GetActiveAxis() == TC::GizmoAxis::Uniform)
            {
                newScale += glm::vec3(scaleDelta);
            }
            else
            {
                switch (gizmo.GetActiveAxis())
                {
                case TC::GizmoAxis::X: newScale.x += scaleDelta; break;
                case TC::GizmoAxis::Y: newScale.y += scaleDelta; break;
                case TC::GizmoAxis::Z: newScale.z += scaleDelta; break;
                default: break;
                }
            }
            newScale = glm::max(newScale, glm::vec3(0.05f));
            object->SetScale(newScale);
        }
    }

    void ProcessHistoryInput(
        GLFWwindow* window,
        TC::Scene& scene,
        TC::TransformGizmo& gizmo,
        SceneHistory& history,
        int& selectedObject)
    {
        bool controlDown =
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        bool zDown = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        bool yDown = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;

        bool undoPressed = controlDown && zDown;
        bool redoPressed = controlDown && yDown;

        if (undoPressed && !g_UndoKeyWasPressed)
        {
            if (history.Undo(scene, selectedObject))
            {
                gizmo.EndDrag();
                gizmo.ClearActiveAxis();
                std::cout << "TC: Undo\n";
            }
            else
            {
                std::cout << "TC: Nothing to undo\n";
            }
        }

        if (redoPressed && !g_RedoKeyWasPressed)
        {
            if (history.Redo(scene, selectedObject))
            {
                gizmo.EndDrag();
                gizmo.ClearActiveAxis();
                std::cout << "TC: Redo\n";
            }
            else
            {
                std::cout << "TC: Nothing to redo\n";
            }
        }

        g_UndoKeyWasPressed = undoPressed;
        g_RedoKeyWasPressed = redoPressed;
    }

    void ProcessSceneFileCommands(
        GLFWwindow* window,
        TC::Scene& scene,
        SceneHistory& history,
        TC::TransformGizmo& gizmo,
        int& selectedObject)
    {
        const bool controlDown =
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        const bool shiftDown =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        const bool saveAsPressed = !g_FileBrowser.IsOpen() && controlDown && shiftDown &&
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        const bool savePressed = !g_FileBrowser.IsOpen() && controlDown && !shiftDown &&
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        const bool loadPressed = !g_FileBrowser.IsOpen() && controlDown &&
            glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

        if (saveAsPressed && !g_SaveAsKeyWasPressed)
        {
            g_FileBrowser.Open(
                TC::FileBrowserMode::SaveAs,
                std::filesystem::path(g_CurrentScenePath).parent_path());
        }
        else if (savePressed && !g_SaveKeyWasPressed)
        {
            if (SaveSceneToFile(scene, g_CurrentScenePath))
                std::cout << "TC: Scene saved to " << g_CurrentScenePath << '\n';
            else
                std::cout << "TC: Failed to save scene to " << g_CurrentScenePath << '\n';
        }
        else if (loadPressed && !g_LoadKeyWasPressed)
        {
            g_FileBrowser.Open(
                TC::FileBrowserMode::Open,
                std::filesystem::path(g_CurrentScenePath).parent_path());
        }

        g_SaveKeyWasPressed = savePressed;
        g_LoadKeyWasPressed = loadPressed;
        g_SaveAsKeyWasPressed = saveAsPressed;

        TC::FileBrowserResult result;
        if (g_FileBrowser.ConsumeResult(result))
        {
            if (result.action == TC::FileBrowserAction::Open)
            {
                if (LoadSceneFromFile(scene, result.path.string()))
                {
                    g_CurrentScenePath = result.path.string();
                    selectedObject = -1;
                    gizmo.EndDrag();
                    gizmo.ClearActiveAxis();
                    history.Initialize(scene, selectedObject);
                    std::cout << "TC: Scene loaded from " << g_CurrentScenePath
                              << " (undo/redo history cleared)\n";
                }
                else
                {
                    std::cout << "TC: Failed to load scene: "
                              << result.path.string() << '\n';
                }
            }
            else if (result.action == TC::FileBrowserAction::Save)
            {
                if (SaveSceneToFile(scene, result.path.string()))
                {
                    g_CurrentScenePath = result.path.string();
                    std::cout << "TC: Scene saved to " << g_CurrentScenePath << '\n';
                }
                else
                {
                    std::cout << "TC: Failed to save scene: "
                              << result.path.string() << '\n';
                }
            }
        }
    }

    void ProcessSceneCommands(
        GLFWwindow* window,
        TC::Scene& scene,
        TC::TransformGizmo& gizmo,
        int& selectedObject
    )
    {
        bool shiftDown =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool dDown = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool duplicatePressed = shiftDown && dDown;

        if (duplicatePressed && !g_DuplicateKeyWasPressed && selectedObject >= 0)
        {
            int duplicateIndex = scene.DuplicateObject(selectedObject);
            if (duplicateIndex >= 0)
            {
                selectedObject = duplicateIndex;
                gizmo.ClearActiveAxis();
                std::cout << "TC: Duplicated object " << duplicateIndex << '\n';
            }
        }
        g_DuplicateKeyWasPressed = duplicatePressed;

        bool deletePressed = glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS;
        if (deletePressed && !g_DeleteKeyWasPressed && selectedObject >= 0)
        {
            if (scene.RemoveObject(selectedObject))
            {
                std::cout << "TC: Deleted selected object\n";
                selectedObject = -1;
                gizmo.ClearActiveAxis();
            }
        }
        g_DeleteKeyWasPressed = deletePressed;

        bool cubePressed = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        if (cubePressed && !g_CreateCubeKeyWasPressed)
        {
            glm::vec3 position(0.0f, 0.5f, 2.0f);
            if (TC::SceneObject* selected = scene.GetObject(selectedObject))
                position = selected->GetPosition() + glm::vec3(1.5f, 0.0f, 0.0f);
            position.y = glm::max(position.y, 0.5f);

            selectedObject = scene.CreateObject(TC::PrimitiveType::Cube, position);
            gizmo.ClearActiveAxis();
            std::cout << "TC: Created cube " << selectedObject << '\n';
        }
        g_CreateCubeKeyWasPressed = cubePressed;

        bool planePressed = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
        if (planePressed && !g_CreatePlaneKeyWasPressed)
        {
            glm::vec3 position(0.0f, 0.02f, 2.0f);
            if (TC::SceneObject* selected = scene.GetObject(selectedObject))
                position = selected->GetPosition() + glm::vec3(1.5f, 0.0f, 0.0f);
            position.y = 0.02f;

            selectedObject = scene.CreateObject(TC::PrimitiveType::Plane, position);
            gizmo.ClearActiveAxis();
            std::cout << "TC: Created plane " << selectedObject << '\n';
        }
        g_CreatePlaneKeyWasPressed = planePressed;

        bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escapePressed && !g_EscapeKeyWasPressed)
        {
            if (selectedObject >= 0)
            {
                selectedObject = -1;
                gizmo.ClearActiveAxis();
                std::cout << "TC: Selection cleared\n";
            }
            else
            {
                glfwSetWindowShouldClose(window, true);
            }
        }
        g_EscapeKeyWasPressed = escapePressed;
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
    SceneHistory history;
    history.Initialize(scene, g_SelectedObject);

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    const bool imguiGlfwReady = ImGui_ImplGlfw_InitForOpenGL(nativeWindow, true);
    const bool imguiOpenGLReady = ImGui_ImplOpenGL3_Init("#version 330");
    std::cout << "TC: ImGui GLFW backend: "
              << (imguiGlfwReady ? "OK" : "FAILED") << '\\n';
    std::cout << "TC: ImGui OpenGL3 backend: "
              << (imguiOpenGLReady ? "OK" : "FAILED") << '\\n';

    if (!imguiGlfwReady || !imguiOpenGLReady)
    {
        std::cerr << "TC: UI initialization failed. Check the OpenGL context and ImGui backend setup.\\n";
        if (imguiGlfwReady)
            ImGui_ImplGlfw_Shutdown();
        if (imguiOpenGLReady)
            ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
        gizmo.Shutdown();
        renderer.Shutdown();
        return -1;
    }

    std::cout << "\nTC Milestone 0.9.9 - Scene Save & Load\n\n";

    std::cout << "Ctrl+Z: Undo | Ctrl+Y: Redo\n";
    std::cout << "Ctrl+S: Save | Ctrl+O: Load | Ctrl+Shift+S: Save As\n";
    std::cout << "Default scene: Assets/Scenes/Untitled.tcscene\n";
    std::cout << "Left Click: Select object / Grab gizmo axis\n";
    std::cout << "Shift+D: Duplicate selected object | Delete: Delete selected object\n";
    std::cout << "1: Create cube | 2: Create plane | Esc: Deselect (Esc again: Exit)\n";
    std::cout << "G: Translation gizmo | T: Rotation gizmo | Y: Scale gizmo\n";
    std::cout << "Left Mouse Drag: Transform along selected axis/handle\n";
    std::cout << "Hold Ctrl: Snap translation (0.5), rotation (15 degrees), scale (0.1)\n";
    std::cout << "Right Mouse: Hold to look around\n";
    std::cout << "WASD: Move camera | Scroll: Zoom\n";
    std::cout << "Space/C: Move camera up/down\n";
    std::cout << "Arrow Keys: Move object X/Z\n";
    std::cout << "Page Up/Down: Move object Y\n";
    std::cout << "Q/E: Rotate | -/+: Uniform scale\n";
    std::cout << "F: Wireframe | R: Reset camera\n\n";

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(nativeWindow))
    {
        float currentFrame = static_cast<float>(glfwGetTime());

        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Use a regular ImGui window for the toolbar rather than the
        // viewport-dependent main menu bar. This remains visible in a
        // single-window GLFW application.
        ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.92f);
        ImGui::Begin("TC Toolbar", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("File"))
            ImGui::OpenPopup("TC File Actions");

        if (ImGui::BeginPopup("TC File Actions"))
        {
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                g_FileBrowser.Open(TC::FileBrowserMode::Open,
                    std::filesystem::path(g_CurrentScenePath).parent_path());

            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                if (SaveSceneToFile(scene, g_CurrentScenePath))
                    std::cout << "TC: Scene saved to " << g_CurrentScenePath << '\\n';
                else
                    std::cout << "TC: Failed to save scene" << '\\n';
            }

            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                g_FileBrowser.Open(TC::FileBrowserMode::SaveAs,
                    std::filesystem::path(g_CurrentScenePath).parent_path());

            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(nativeWindow, true);

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("Ctrl+O: Open   Ctrl+S: Save   Ctrl+Shift+S: Save As");
        ImGui::End();

        ProcessSceneFileCommands(
            nativeWindow, scene, history, gizmo, g_SelectedObject);

        if (!g_FileBrowser.IsOpen())
        {
            ProcessHistoryInput(
                nativeWindow, scene, gizmo, history, g_SelectedObject);

            if (!g_FileBrowser.IsOpen())
            {
                ProcessSceneCommands(
                    nativeWindow, scene, gizmo, g_SelectedObject);
                ProcessCameraInput(nativeWindow, camera, deltaTime);
            }
        }

        bool translatePressed = !g_FileBrowser.IsOpen() && glfwGetKey(nativeWindow, GLFW_KEY_G) == GLFW_PRESS;
        if (translatePressed && !g_TranslateKeyWasPressed && !gizmo.IsDragging())
        {
            gizmo.SetMode(TC::GizmoMode::Translate);
            std::cout << "TC: Translation gizmo selected\n";
        }
        g_TranslateKeyWasPressed = translatePressed;

        bool rotatePressed = !g_FileBrowser.IsOpen() && glfwGetKey(nativeWindow, GLFW_KEY_T) == GLFW_PRESS;
        if (rotatePressed && !g_RotateKeyWasPressed && !gizmo.IsDragging())
        {
            gizmo.SetMode(TC::GizmoMode::Rotate);
            std::cout << "TC: Rotation gizmo selected\n";
        }
        g_RotateKeyWasPressed = rotatePressed;

        bool scalePressed = !g_FileBrowser.IsOpen() && glfwGetKey(nativeWindow, GLFW_KEY_Y) == GLFW_PRESS;
        bool controlDownForShortcuts =
            glfwGetKey(nativeWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(nativeWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        if (scalePressed && !controlDownForShortcuts && !g_ScaleKeyWasPressed && !gizmo.IsDragging())
        {
            gizmo.SetMode(TC::GizmoMode::Scale);
            std::cout << "TC: Scale gizmo selected\n";
        }
        g_ScaleKeyWasPressed = scalePressed;

        if (!g_FileBrowser.IsOpen() && gizmo.IsDragging())
        {
            ProcessGizmoDragging(
                nativeWindow,
                scene,
                gizmo,
                g_SelectedObject
            );
        }
        else if (!g_FileBrowser.IsOpen())
        {
            ProcessObjectTransform(
                nativeWindow,
                scene,
                g_SelectedObject,
                deltaTime
            );
        }

        history.Observe(scene, g_SelectedObject, glfwGetTime());

        bool wireframePressed = !g_FileBrowser.IsOpen() &&
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

        bool resetPressed = !g_FileBrowser.IsOpen() &&
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

            if (selected)
            {
                gizmo.Render(
                    camera,
                    selected->GetPosition(),
                    aspectRatio
                );
            }
        }

        g_FileBrowser.Render();

        ImGui::Render();

        // Reassert the default framebuffer and framebuffer viewport before
        // the UI backend submits its draw lists.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(nativeWindow);
        glfwPollEvents();
    }

    gizmo.EndDrag();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    gizmo.Shutdown();
    renderer.Shutdown();

    g_Camera = nullptr;
    g_Scene = nullptr;
    g_Gizmo = nullptr;

    std::cout << "TC: Application shutting down!\n";

    return 0;
}