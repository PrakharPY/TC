#include "Renderer.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>

namespace TC
{
    bool Renderer::Initialize()
    {
        if (!m_Shader.LoadFromFiles(
            "Assets/Shaders/basic.vert",
            "Assets/Shaders/basic.frag"))
        {
            std::cerr << "Failed to load shaders!\n";
            return false;
        }

        // ------------------------------------------------
        // CUBE GEOMETRY
        // Position (3 floats) + Normal (3 floats)
        // ------------------------------------------------

        float cubeVertices[] =
        {
            // Front face
            -0.5f, -0.5f,  0.5f,  0, 0, 1,
             0.5f, -0.5f,  0.5f,  0, 0, 1,
             0.5f,  0.5f,  0.5f,  0, 0, 1,
            -0.5f,  0.5f,  0.5f,  0, 0, 1,

            // Back face
             0.5f, -0.5f, -0.5f,  0, 0,-1,
            -0.5f, -0.5f, -0.5f,  0, 0,-1,
            -0.5f,  0.5f, -0.5f,  0, 0,-1,
             0.5f,  0.5f, -0.5f,  0, 0,-1,

            // Left face
            -0.5f, -0.5f, -0.5f, -1, 0, 0,
            -0.5f, -0.5f,  0.5f, -1, 0, 0,
            -0.5f,  0.5f,  0.5f, -1, 0, 0,
            -0.5f,  0.5f, -0.5f, -1, 0, 0,

            // Right face
             0.5f, -0.5f,  0.5f,  1, 0, 0,
             0.5f, -0.5f, -0.5f,  1, 0, 0,
             0.5f,  0.5f, -0.5f,  1, 0, 0,
             0.5f,  0.5f,  0.5f,  1, 0, 0,

            // Top face
            -0.5f,  0.5f,  0.5f,  0, 1, 0,
             0.5f,  0.5f,  0.5f,  0, 1, 0,
             0.5f,  0.5f, -0.5f,  0, 1, 0,
            -0.5f,  0.5f, -0.5f,  0, 1, 0,

            // Bottom face
            -0.5f, -0.5f, -0.5f,  0,-1, 0,
             0.5f, -0.5f, -0.5f,  0,-1, 0,
             0.5f, -0.5f,  0.5f,  0,-1, 0,
            -0.5f, -0.5f,  0.5f,  0,-1, 0
        };

        unsigned int cubeIndices[] =
        {
             0,  1,  2,  2,  3,  0,
             4,  5,  6,  6,  7,  4,
             8,  9, 10, 10, 11,  8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        glGenVertexArrays(1, &m_CubeVAO);
        glGenBuffers(1, &m_CubeVBO);
        glGenBuffers(1, &m_CubeEBO);

        glBindVertexArray(m_CubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(cubeVertices),
            cubeVertices,
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeEBO);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(cubeIndices),
            cubeIndices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float), (void*)0
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float),
            (void*)(3 * sizeof(float))
        );

        glEnableVertexAttribArray(1);

        // ------------------------------------------------
        // GROUND PLANE
        // ------------------------------------------------

        float planeVertices[] =
        {
            -0.5f, 0.0f, -0.5f,  0, 1, 0,
             0.5f, 0.0f, -0.5f,  0, 1, 0,
             0.5f, 0.0f,  0.5f,  0, 1, 0,
            -0.5f, 0.0f,  0.5f,  0, 1, 0
        };

        unsigned int planeIndices[] =
        {
            0, 2, 1,
            0, 3, 2
        };

        glGenVertexArrays(1, &m_PlaneVAO);
        glGenBuffers(1, &m_PlaneVBO);
        glGenBuffers(1, &m_PlaneEBO);

        glBindVertexArray(m_PlaneVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_PlaneVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(planeVertices),
            planeVertices,
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_PlaneEBO);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(planeIndices),
            planeIndices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float), (void*)0
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            6 * sizeof(float),
            (void*)(3 * sizeof(float))
        );

        glEnableVertexAttribArray(1);

        // ------------------------------------------------
        // GRID
        // ------------------------------------------------

        std::vector<float> gridVertices;

        for (int i = -10; i <= 10; ++i)
        {
            float position = static_cast<float>(i);

            gridVertices.insert(gridVertices.end(),
            {
                -10.0f, 0.015f, position,
                 10.0f, 0.015f, position
            });

            gridVertices.insert(gridVertices.end(),
            {
                position, 0.015f, -10.0f,
                position, 0.015f,  10.0f
            });
        }

        glGenVertexArrays(1, &m_GridVAO);
        glGenBuffers(1, &m_GridVBO);

        glBindVertexArray(m_GridVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            gridVertices.size() * sizeof(float),
            gridVertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0
        );

        glEnableVertexAttribArray(0);

        // ------------------------------------------------
        // COORDINATE AXES
        // ------------------------------------------------

        float axisVertices[] =
        {
            -10.0f, 0.02f, 0.0f,
             10.0f, 0.02f, 0.0f,

             0.0f, 0.02f,  0.0f,
             0.0f, 10.0f,  0.0f,

             0.0f, 0.02f, -10.0f,
             0.0f, 0.02f,  10.0f
        };

        glGenVertexArrays(1, &m_AxisVAO);
        glGenBuffers(1, &m_AxisVBO);

        glBindVertexArray(m_AxisVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_AxisVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(axisVertices),
            axisVertices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0
        );

        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);

        return true;
    }

    void Renderer::DrawSceneObject(const SceneObject& object)
    {
        glm::mat4 model = object.GetModelMatrix();

        m_Shader.SetMat4("u_Model", model);
        m_Shader.SetVec3("u_Color", object.GetColor());

        if (object.GetType() == PrimitiveType::Cube)
        {
            glBindVertexArray(m_CubeVAO);

            glDrawElements(
                GL_TRIANGLES,
                36,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
        else if (object.GetType() == PrimitiveType::Plane)
        {
            glBindVertexArray(m_PlaneVAO);

            glDrawElements(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    }

    void Renderer::Render(
        const Camera& camera,
        const Scene& scene,
        float aspectRatio
    )
    {
        glClearColor(0.055f, 0.06f, 0.09f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        m_Shader.Use();

        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection =
            camera.GetProjectionMatrix(aspectRatio);

        m_Shader.SetMat4("u_View", view);
        m_Shader.SetMat4("u_Projection", projection);

        // Lighting configuration
        m_Shader.SetVec3(
            "u_ViewPosition",
            camera.GetPosition()
        );

        m_Shader.SetVec3(
            "u_LightDirection",
            glm::vec3(-0.5f, -1.0f, -0.3f)
        );

        m_Shader.SetVec3(
            "u_LightColor",
            glm::vec3(1.0f, 1.0f, 1.0f)
        );

        m_Shader.SetFloat("u_AmbientStrength", 0.28f);
        m_Shader.SetFloat("u_SpecularStrength", 0.35f);

        m_Shader.SetInt("u_LightingEnabled", 1);

        // Render scene objects
        glPolygonMode(
            GL_FRONT_AND_BACK,
            m_Wireframe ? GL_LINE : GL_FILL
        );

        for (const SceneObject& object : scene.GetObjects())
        {
            DrawSceneObject(object);
        }

        // Render grid without lighting
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glm::mat4 identity(1.0f);

        m_Shader.SetMat4("u_Model", identity);

        m_Shader.SetInt("u_LightingEnabled", 0);

        m_Shader.SetVec3(
            "u_Color",
            glm::vec3(0.35f, 0.37f, 0.42f)
        );

        glBindVertexArray(m_GridVAO);

        glDrawArrays(GL_LINES, 0, 84);

        // X axis - Red
        m_Shader.SetVec3(
            "u_Color",
            glm::vec3(1.0f, 0.1f, 0.1f)
        );

        glBindVertexArray(m_AxisVAO);

        glDrawArrays(GL_LINES, 0, 2);

        // Y axis - Green
        m_Shader.SetVec3(
            "u_Color",
            glm::vec3(0.1f, 1.0f, 0.1f)
        );

        glDrawArrays(GL_LINES, 2, 2);

        // Z axis - Blue
        m_Shader.SetVec3(
            "u_Color",
            glm::vec3(0.1f, 0.3f, 1.0f)
        );

        glDrawArrays(GL_LINES, 4, 2);

        glBindVertexArray(0);

        m_Shader.SetInt("u_LightingEnabled", 1);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void Renderer::SetWireframe(bool enabled)
    {
        m_Wireframe = enabled;
    }

    void Renderer::Shutdown()
    {
        glDeleteVertexArrays(1, &m_CubeVAO);
        glDeleteBuffers(1, &m_CubeVBO);
        glDeleteBuffers(1, &m_CubeEBO);

        glDeleteVertexArrays(1, &m_PlaneVAO);
        glDeleteBuffers(1, &m_PlaneVBO);
        glDeleteBuffers(1, &m_PlaneEBO);

        glDeleteVertexArrays(1, &m_GridVAO);
        glDeleteBuffers(1, &m_GridVBO);

        glDeleteVertexArrays(1, &m_AxisVAO);
        glDeleteBuffers(1, &m_AxisVBO);
    }
}