#include "SceneObject.h"

#include <glm/gtc/matrix_transform.hpp>

namespace TC
{
    SceneObject::SceneObject(
        PrimitiveType type,
        const glm::vec3& position,
        const glm::vec3& rotation,
        const glm::vec3& scale,
        const glm::vec3& color
    )
        : m_Type(type),
          m_Position(position),
          m_Rotation(rotation),
          m_Scale(scale),
          m_Color(color)
    {
    }

    glm::mat4 SceneObject::GetModelMatrix() const
    {
        glm::mat4 model(1.0f);

        model = glm::translate(model, m_Position);

        model = glm::rotate(
            model,
            glm::radians(m_Rotation.x),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        model = glm::rotate(
            model,
            glm::radians(m_Rotation.y),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        model = glm::rotate(
            model,
            glm::radians(m_Rotation.z),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        model = glm::scale(model, m_Scale);

        return model;
    }

    PrimitiveType SceneObject::GetType() const
    {
        return m_Type;
    }

    glm::vec3 SceneObject::GetColor() const
    {
        return m_Color;
    }

    glm::vec3 SceneObject::GetPosition() const
    {
        return m_Position;
    }

    glm::vec3 SceneObject::GetRotation() const
    {
        return m_Rotation;
    }

    glm::vec3 SceneObject::GetScale() const
    {
        return m_Scale;
    }

    void SceneObject::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
    }

    void SceneObject::SetRotation(const glm::vec3& rotation)
    {
        m_Rotation = rotation;
    }

    void SceneObject::SetScale(const glm::vec3& scale)
    {
        m_Scale = scale;
    }

    void SceneObject::SetColor(const glm::vec3& color)
    {
        m_Color = color;
    }

    void SceneObject::Translate(const glm::vec3& offset)
    {
        m_Position += offset;
    }

    void SceneObject::Rotate(const glm::vec3& offset)
    {
        m_Rotation += offset;
    }

    void SceneObject::ScaleBy(float factor)
    {
        m_Scale *= factor;

        // Prevent zero or negative scale.
        m_Scale = glm::max(
            m_Scale,
            glm::vec3(0.05f)
        );
    }
}