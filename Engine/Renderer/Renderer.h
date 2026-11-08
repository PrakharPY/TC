#pragma once

#include "Shader.h"
#include "../Camera/Camera.h"

namespace TC
{
    class Renderer
    {
    public:
        bool Initialize();

        void Render(
            const Camera& camera,
            float aspectRatio
        );

        void SetWireframe(bool enabled);

        void Shutdown();

    private:
        Shader m_Shader;

        unsigned int m_VAO = 0;
        unsigned int m_VBO = 0;
        unsigned int m_EBO = 0;

        bool m_Wireframe = false;
    };
}