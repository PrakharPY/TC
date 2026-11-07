#pragma once

#include "Shader.h"

namespace TC
{
    class Renderer
    {
    public:
        bool Initialize();

        void Render();

        void Shutdown();

    private:
        Shader m_Shader;

        unsigned int m_VAO = 0;
        unsigned int m_VBO = 0;
    };
}