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

        GizmoAxis GetHoveredAxis() const;
        GizmoAxis GetActiveAxis() const;

        void SetActiveAxis(GizmoAxis axis);
        void ClearActiveAxis();

        void Shutdown();

    private:
        Shader m_Shader;

        unsigned int m_VAO = 0;
        unsigned int m_VBO = 0;

        GizmoAxis m_HoveredAxis = GizmoAxis::None;
        GizmoAxis m_ActiveAxis = GizmoAxis::None;
    };
}