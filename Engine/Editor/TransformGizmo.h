#pragma once

#include "../Renderer/Shader.h"
#include "../Camera/Camera.h"

#include <glm/glm.hpp>

namespace TC
{
    enum class GizmoAxis
    {
        None,
        X,
        Y,
        Z
    };

    class TransformGizmo
    {
    public:
        bool Initialize();

        void Render(
            const Camera& camera,
            const glm::vec3& position,
            float aspectRatio
        );

        void UpdateHover(
            const Camera& camera,
            const glm::vec3& position,
            float mouseX,
            float mouseY,
            int viewportWidth,
            int viewportHeight
        );

        bool BeginDrag(
            const Camera& camera,
            const glm::vec3& position,
            float mouseX,
            float mouseY,
            int viewportWidth,
            int viewportHeight
        );

        glm::vec3 UpdateDrag(
            const Camera& camera,
            float mouseX,
            float mouseY,
            int viewportWidth,
            int viewportHeight
        );

        void EndDrag();

        bool IsDragging() const;

        GizmoAxis GetHoveredAxis() const;
        GizmoAxis GetActiveAxis() const;

        void SetActiveAxis(GizmoAxis axis);
        void ClearActiveAxis();

        void Shutdown();

    private:
        bool GetAxisParameter(
            const Camera& camera,
            const glm::vec3& origin,
            GizmoAxis axis,
            float mouseX,
            float mouseY,
            int viewportWidth,
            int viewportHeight,
            float& parameter
        ) const;

        Shader m_Shader;

        unsigned int m_VAO = 0;
        unsigned int m_VBO = 0;

        GizmoAxis m_HoveredAxis = GizmoAxis::None;
        GizmoAxis m_ActiveAxis = GizmoAxis::None;

        bool m_IsDragging = false;

        glm::vec3 m_DragOrigin = glm::vec3(0.0f);

        float m_LastDragParameter = 0.0f;
    };
}