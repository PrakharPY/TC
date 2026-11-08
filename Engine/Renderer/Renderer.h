#pragma once

#include "Shader.h"
#include "../Camera/Camera.h"
#include "../Scene/Scene.h"

namespace TC
{
    class Renderer
    {
    public:
        bool Initialize();

        void Render(
            const Camera& camera,
            const Scene& scene,
            float aspectRatio
        );

        void SetWireframe(bool enabled);

        void Shutdown();

    private:
        void DrawSceneObject(const SceneObject& object);

        Shader m_Shader;

        unsigned int m_CubeVAO = 0;
        unsigned int m_CubeVBO = 0;
        unsigned int m_CubeEBO = 0;

        unsigned int m_PlaneVAO = 0;
        unsigned int m_PlaneVBO = 0;
        unsigned int m_PlaneEBO = 0;

        unsigned int m_GridVAO = 0;
        unsigned int m_GridVBO = 0;

        unsigned int m_AxisVAO = 0;
        unsigned int m_AxisVBO = 0;

        bool m_Wireframe = false;
    };
}