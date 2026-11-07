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
        const std::string& fragmentPath)
    {
        auto ReadFile = [](const std::string& path)
        {
            std::ifstream file(path);
            std::stringstream buffer;

            if (!file)
            {
                std::cerr << "TC: Cannot open shader: "
                          << path << "\n";

                return std::string{};
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
                char info[512];

                glGetShaderInfoLog(
                    shader, 512, nullptr, info
                );

                std::cerr << "TC: Shader compilation failed:\n"
                          << info << "\n";

                glDeleteShader(shader);

                return GLuint(0);
            }

            return shader;
        };

        GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource);
        GLuint fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource);

        if (!vertex || !fragment)
        {
            if (vertex) glDeleteShader(vertex);
            if (fragment) glDeleteShader(fragment);

            return false;
        }

        GLuint program = glCreateProgram();

        glAttachShader(program, vertex);
        glAttachShader(program, fragment);

        glLinkProgram(program);

        GLint success = 0;

        glGetProgramiv(program, GL_LINK_STATUS, &success);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        if (!success)
        {
            char info[512];

            glGetProgramInfoLog(
                program, 512, nullptr, info
            );

            std::cerr << "TC: Shader linking failed:\n"
                      << info << "\n";

            glDeleteProgram(program);

            return false;
        }

        if (m_Program)
            glDeleteProgram(m_Program);

        m_Program = program;

        std::cout << "TC: Shader loaded successfully!\n";

        return true;
    }

    void Shader::Use() const
    {
        glUseProgram(m_Program);
    }

    void Shader::SetMat4(
        const std::string& name,
        const glm::mat4& matrix) const
    {
        GLint location = glGetUniformLocation(
            m_Program,
            name.c_str()
        );

        if (location == -1)
            return;

        glUniformMatrix4fv(
            location,
            1,
            GL_FALSE,
            glm::value_ptr(matrix)
        );
    }
}