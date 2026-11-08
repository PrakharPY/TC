#include "Renderer.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace TC
{
    bool Renderer::Initialize()
    {
        float vertices[] =
        {
            // Position             // Color
            -0.5f,-0.5f,-0.5f,      1,0,0,
             0.5f,-0.5f,-0.5f,      0,1,0,
             0.5f, 0.5f,-0.5f,      0,0,1,
            -0.5f, 0.5f,-0.5f,      1,1,0,

            -0.5f,-0.5f, 0.5f,      1,0,1,
             0.5f,-0.5f, 0.5f,      0,1,1,
             0.5f, 0.5f, 0.5f,      1,1,1,
            -0.5f, 0.5f, 0.5f,      1,0.5f,0
        };

        unsigned int indices[] =
        {
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            0,4,7, 7,3,0,
            1,5,6, 6,2,1,
            3,2,6, 6,7,3,
            0,1,5, 5,4,0
        };

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(indices),
            indices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float), nullptr
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float),
            (void*)(3 * sizeof(float))
        );

        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);

        if (!m_Shader.LoadFromFiles(
            "Assets/Shaders/basic.vert",
            "Assets/Shaders/basic.frag"))
        {
            return false;
        }

        std::cout << "TC: Renderer initialized!\n";

        return true;
    }

    void Renderer::Render(
        const Camera& camera,
        float aspectRatio)
    {
        glClearColor(
            0.06f, 0.07f, 0.11f, 1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        m_Shader.Use();

        glm::mat4 model(1.0f);

        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection =
            camera.GetProjectionMatrix(aspectRatio);

        glm::mat4 mvp = projection * view * model;

        m_Shader.SetMat4("uMVP", mvp);

        // Wireframe rendering
        glPolygonMode(
            GL_FRONT_AND_BACK,
            m_Wireframe ? GL_LINE : GL_FILL
        );

        glBindVertexArray(m_VAO);

        glDrawElements(
            GL_TRIANGLES,
            36,
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);

        // Restore default rendering mode
        glPolygonMode(
            GL_FRONT_AND_BACK,
            GL_FILL
        );
    }

    void Renderer::SetWireframe(bool enabled)
    {
        m_Wireframe = enabled;
    }

    void Renderer::Shutdown()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }
}