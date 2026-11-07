#pragma once

#include <glad/glad.h>
#include <string>

namespace TC
{
    class Shader
    {
    public:
        Shader() = default;
        ~Shader();

        bool LoadFromFiles(
            const std::string& vertexPath,
            const std::string& fragmentPath
        );

        void Use() const;

    private:
        GLuint m_Program = 0;
    };
}