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

        void SetMat4(
            const std::string& name,
            const glm::mat4& matrix
        ) const;

        void SetVec3(
            const std::string& name,
            const glm::vec3& value
        ) const;

        void SetFloat(
            const std::string& name,
            float value
        ) const;

        void SetInt(
            const std::string& name,
            int value
        ) const;

    private:
        GLuint m_Program = 0;
    };
}