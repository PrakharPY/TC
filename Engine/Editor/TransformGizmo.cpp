#include "TransformGizmo.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cmath>

namespace TC
{
    namespace
    {
        bool ProjectToScreen(
            const glm::vec3& worldPosition,
            const glm::mat4& view,
            const glm::mat4& projection,
            int width,
            int height,
            glm::vec2& screenPosition
        )
        {
            glm::vec4 clip =
                projection * view * glm::vec4(worldPosition, 1.0f);

            if (clip.w <= 0.0001f)
                return false;

            glm::vec3 ndc = glm::vec3(clip) / clip.w;

            screenPosition.x =
                (ndc.x * 0.5f + 0.5f) * width;

            screenPosition.y =
                (1.0f - (ndc.y * 0.5f + 0.5f)) * height;

            return true;
        }

        float DistanceToSegment(
            const glm::vec2& point,
            const glm::vec2& start,
            const glm::vec2& end
        )
        {
            glm::vec2 segment = end - start;

            float lengthSquared = glm::dot(segment, segment);

            if (lengthSquared < 0.0001f)
                return glm::length(point - start);

            float t = glm::dot(point - start, segment)
                / lengthSquared;

            t = glm::clamp(t, 0.0f, 1.0f);

            glm::vec2 closest = start + t * segment;

            return glm::length(point - closest);
        }

        glm::vec3 GetAxisDirection(GizmoAxis axis)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                return glm::vec3(1.0f, 0.0f, 0.0f);

            case GizmoAxis::Y:
                return glm::vec3(0.0f, 1.0f, 0.0f);

            case GizmoAxis::Z:
                return glm::vec3(0.0f, 0.0f, 1.0f);

            default:
                return glm::vec3(0.0f);
            }
        }

        glm::vec3 ScreenPointToRay(
            const Camera& camera,
            float mouseX,
            float mouseY,
            int width,
            int height
        )
        {
            float x = (2.0f * mouseX) / width - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / height;

            glm::vec4 rayClip(x, y, -1.0f, 1.0f);

            glm::mat4 projection =
                camera.GetProjectionMatrix(
                    static_cast<float>(width) /
                    static_cast<float>(height)
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
    }

    bool TransformGizmo::Initialize()
    {
        if (!m_Shader.LoadFromFiles(
            "Assets/Shaders/basic.vert",
            "Assets/Shaders/basic.frag"))
        {
            std::cerr << "Failed to load gizmo shaders!\n";
            return false;
        }

        float vertices[] =
        {
            // X axis
            0.0f, 0.0f, 0.0f,
            1.3f, 0.0f, 0.0f,

            // X arrowhead
            1.3f, 0.0f, 0.0f,
            1.12f, 0.08f, 0.0f,
            1.12f, 0.0f, -0.08f,

            // Y axis
            0.0f, 0.0f, 0.0f,
            0.0f, 1.3f, 0.0f,

            // Y arrowhead
            0.0f, 1.3f, 0.0f,
            0.08f, 1.12f, 0.0f,
            0.0f, 1.12f, 0.08f,

            // Z axis
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.3f,

            // Z arrowhead
            0.0f, 0.0f, 1.3f,
            0.08f, 0.0f, 1.12f,
            0.0f, 0.08f, 1.12f
        };

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)0
        );

        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        std::cout << "TC: Interactive transform gizmo initialized!\n";

        return true;
    }

    bool TransformGizmo::GetAxisParameter(
        const Camera& camera,
        const glm::vec3& origin,
        GizmoAxis axis,
        float mouseX,
        float mouseY,
        int viewportWidth,
        int viewportHeight,
        float& parameter
    ) const
    {
        if (viewportWidth <= 0 || viewportHeight <= 0 ||
            axis == GizmoAxis::None)
        {
            return false;
        }

        glm::vec3 axisDirection = GetAxisDirection(axis);

        glm::vec3 rayOrigin = camera.GetPosition();

        glm::vec3 rayDirection = ScreenPointToRay(
            camera,
            mouseX,
            mouseY,
            viewportWidth,
            viewportHeight
        );

        // Calculate the closest points between:
        // 1. The selected world-space axis.
        // 2. The mouse ray.

        glm::vec3 w = origin - rayOrigin;

        float a = glm::dot(axisDirection, axisDirection);
        float b = glm::dot(axisDirection, rayDirection);
        float c = glm::dot(rayDirection, rayDirection);

        float d = glm::dot(axisDirection, w);
        float e = glm::dot(rayDirection, w);

        float denominator = a * c - b * b;

        // Avoid unstable calculations when the ray is
        // almost parallel to the selected axis.
        if (std::abs(denominator) < 0.0001f)
            return false;

        parameter = (b * e - c * d) / denominator;

        return true;
    }

    void TransformGizmo::UpdateHover(
        const Camera& camera,
        const glm::vec3& position,
        float mouseX,
        float mouseY,
        int viewportWidth,
        int viewportHeight
    )
    {
        if (m_IsDragging)
            return;

        m_HoveredAxis = GizmoAxis::None;

        if (viewportWidth <= 0 || viewportHeight <= 0)
            return;

        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection =
            camera.GetProjectionMatrix(
                static_cast<float>(viewportWidth) /
                static_cast<float>(viewportHeight)
            );

        glm::vec2 mouse(mouseX, mouseY);
        glm::vec2 start;
        glm::vec2 end;

        const float hoverDistance = 12.0f;

        float closestDistance = hoverDistance;

        glm::vec3 axes[] =
        {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        };

        GizmoAxis axisTypes[] =
        {
            GizmoAxis::X,
            GizmoAxis::Y,
            GizmoAxis::Z
        };

        if (!ProjectToScreen(
            position,
            view,
            projection,
            viewportWidth,
            viewportHeight,
            start))
        {
            return;
        }

        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 endpoint =
                position + axes[i] * 1.3f;

            if (!ProjectToScreen(
                endpoint,
                view,
                projection,
                viewportWidth,
                viewportHeight,
                end))
            {
                continue;
            }

            float distance =
                DistanceToSegment(mouse, start, end);

            if (distance < closestDistance)
            {
                closestDistance = distance;
                m_HoveredAxis = axisTypes[i];
            }
        }
    }

    bool TransformGizmo::BeginDrag(
        const Camera& camera,
        const glm::vec3& position,
        float mouseX,
        float mouseY,
        int viewportWidth,
        int viewportHeight
    )
    {
        if (m_HoveredAxis == GizmoAxis::None)
            return false;

        float parameter = 0.0f;

        if (!GetAxisParameter(
            camera,
            position,
            m_HoveredAxis,
            mouseX,
            mouseY,
            viewportWidth,
            viewportHeight,
            parameter))
        {
            return false;
        }

        m_DragOrigin = position;
        m_LastDragParameter = parameter;

        m_ActiveAxis = m_HoveredAxis;
        m_IsDragging = true;

        std::cout << "TC: Started dragging axis\n";

        return true;
    }

    glm::vec3 TransformGizmo::UpdateDrag(
        const Camera& camera,
        float mouseX,
        float mouseY,
        int viewportWidth,
        int viewportHeight
    )
    {
        if (!m_IsDragging)
            return glm::vec3(0.0f);

        float currentParameter = 0.0f;

        if (!GetAxisParameter(
            camera,
            m_DragOrigin,
            m_ActiveAxis,
            mouseX,
            mouseY,
            viewportWidth,
            viewportHeight,
            currentParameter))
        {
            return glm::vec3(0.0f);
        }

        float movement =
            currentParameter - m_LastDragParameter;

        m_LastDragParameter = currentParameter;

        return GetAxisDirection(m_ActiveAxis) * movement;
    }

    void TransformGizmo::EndDrag()
    {
        if (m_IsDragging)
            std::cout << "TC: Finished dragging axis\n";

        m_IsDragging = false;
    }

    bool TransformGizmo::IsDragging() const
    {
        return m_IsDragging;
    }

    void TransformGizmo::Render(
        const Camera& camera,
        const glm::vec3& position,
        float aspectRatio
    )
    {
        m_Shader.Use();

        glm::mat4 model(1.0f);
        model = glm::translate(model, position);

        m_Shader.SetMat4("u_Model", model);
        m_Shader.SetMat4("u_View", camera.GetViewMatrix());

        m_Shader.SetMat4(
            "u_Projection",
            camera.GetProjectionMatrix(aspectRatio)
        );

        m_Shader.SetInt("u_LightingEnabled", 0);

        glBindVertexArray(m_VAO);

        glDisable(GL_DEPTH_TEST);

        glLineWidth(3.0f);

        auto DrawAxis = [&](int lineIndex,
                            int triangleIndex,
                            GizmoAxis axis,
                            const glm::vec3& baseColor)
        {
            glm::vec3 color = baseColor;

            if (m_ActiveAxis == axis)
            {
                color = glm::vec3(1.0f, 0.65f, 0.05f);
            }
            else if (m_HoveredAxis == axis)
            {
                color = glm::vec3(1.0f, 1.0f, 0.15f);
            }

            m_Shader.SetVec3("u_Color", color);

            glDrawArrays(GL_LINES, lineIndex, 2);
            glDrawArrays(GL_TRIANGLES, triangleIndex, 3);
        };

        DrawAxis(
            0, 2, GizmoAxis::X,
            glm::vec3(1.0f, 0.1f, 0.1f)
        );

        DrawAxis(
            5, 7, GizmoAxis::Y,
            glm::vec3(0.1f, 1.0f, 0.1f)
        );

        DrawAxis(
            10, 12, GizmoAxis::Z,
            glm::vec3(0.1f, 0.3f, 1.0f)
        );

        glLineWidth(1.0f);

        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }

    GizmoAxis TransformGizmo::GetHoveredAxis() const
    {
        return m_HoveredAxis;
    }

    GizmoAxis TransformGizmo::GetActiveAxis() const
    {
        return m_ActiveAxis;
    }

    void TransformGizmo::SetActiveAxis(GizmoAxis axis)
    {
        m_ActiveAxis = axis;
    }

    void TransformGizmo::ClearActiveAxis()
    {
        m_ActiveAxis = GizmoAxis::None;
    }

    void TransformGizmo::Shutdown()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);

        m_VAO = 0;
        m_VBO = 0;

        m_IsDragging = false;
    }
}