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

        std::cout << "TC: Interactive gizmo initialized!\n";

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
    }
}