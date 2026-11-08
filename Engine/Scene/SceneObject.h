#pragma once

#include <glm/glm.hpp>

namespace TC
{
    enum class PrimitiveType
    {
        Cube,
        Plane
    };

    class SceneObject
    {
    public:
        SceneObject(
            PrimitiveType type,
            const glm::vec3& position,
            const glm::vec3& rotation,
            const glm::vec3& scale,
            const glm::vec3& color
        );

        glm::mat4 GetModelMatrix() const;

        PrimitiveType GetType() const;
        glm::vec3 GetColor() const;

        glm::vec3 GetPosition() const;
        glm::vec3 GetRotation() const;
        glm::vec3 GetScale() const;

        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);
        void SetScale(const glm::vec3& scale);
        void SetColor(const glm::vec3& color);

        void Translate(const glm::vec3& offset);
        void Rotate(const glm::vec3& offset);
        void ScaleBy(float factor);

    private:
        PrimitiveType m_Type;

        glm::vec3 m_Position;
        glm::vec3 m_Rotation;
        glm::vec3 m_Scale;
        glm::vec3 m_Color;
    };
}