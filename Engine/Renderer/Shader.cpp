#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

namespace TC
{
    Shader::~Shader()
    {
        if (m_Program)
            glDeleteProgram(m_Program);
    }

    bool Shader::LoadFromFiles(
        const std::string& vertexPath,
        const std::string& fragmentPath
    )
    {
        auto ReadFile = [](const std::string& path)
        {
            std::ifstream file(path);
            std::stringstream buffer;

            if (!file.is_open())
            {
                std::cerr << "Failed to open shader: "
                          << path << '\n';
            }

            buffer << file.rdbuf();
            return buffer.str();
        };

        std::string vertexSource = ReadFile(vertexPath);
        std::string fragmentSource = ReadFile(fragmentPath);

        if (vertexSource.empty() || fragmentSource.empty())
            return false;

        auto Compile = [](GLenum type, const std::string& source)
        {
            GLuint shader = glCreateShader(type);

            const char* code = source.c_str();

            glShaderSource(shader, 1, &code, nullptr);
            glCompileShader(shader);

            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success)
            {
                char infoLog[512];

                glGetShaderInfoLog(
                    shader,
                    512,
                    nullptr,
                    infoLog
                );

                std::cerr << "Shader compilation failed:\n"
                          << infoLog << '\n';

                glDeleteShader(shader);
                return GLuint(0);
            }

            return shader;
        };

        GLuint vertexShader =
            Compile(GL_VERTEX_SHADER, vertexSource);

        if (!vertexShader)
            return false;

        GLuint fragmentShader =
            Compile(GL_FRAGMENT_SHADER, fragmentSource);

        if (!fragmentShader)
        {
            glDeleteShader(vertexShader);
            return false;
        }

        GLuint program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);

        GLint success = 0;

        glGetProgramiv(
            program,
            GL_LINK_STATUS,
            &success
        );

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        if (!success)
        {
            char infoLog[512];

            glGetProgramInfoLog(
                program,
                512,
                nullptr,
                infoLog
            );

            std::cerr << "Shader linking failed:\n"
                      << infoLog << '\n';

            glDeleteProgram(program);
            return false;
        }

        if (m_Program)
            glDeleteProgram(m_Program);

        m_Program = program;

        return true;
    }

    void Shader::Use() const
    {
        glUseProgram(m_Program);
    }

    void Shader::SetMat4(
        const std::string& name,
        const glm::mat4& matrix
    ) const
    {
        GLint location =
            glGetUniformLocation(m_Program, name.c_str());

        if (location == -1)
            return;

        glUniformMatrix4fv(
            location,
            1,
            GL_FALSE,
            glm::value_ptr(matrix)
        );
    }

    void Shader::SetVec3(
        const std::string& name,
        const glm::vec3& value
    ) const
    {
        GLint location =
            glGetUniformLocation(m_Program, name.c_str());

        if (location == -1)
            return;

        glUniform3fv(
            location,
            1,
            glm::value_ptr(value)
        );
    }

    void Shader::SetFloat(
        const std::string& name,
        float value
    ) const
    {
        GLint location =
            glGetUniformLocation(m_Program, name.c_str());

        if (location == -1)
            return;

        glUniform1f(location, value);
    }

    void Shader::SetInt(
        const std::string& name,
        int value
    ) const
    {
        GLint location =
            glGetUniformLocation(m_Program, name.c_str());

        if (location == -1)
            return;

        glUniform1i(location, value);
    }
}