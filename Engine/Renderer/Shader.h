#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
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
        void SetMat4(const std::string& name, const glm::mat4& matrix) const;

    private:
        GLuint m_Program = 0;
    };
}