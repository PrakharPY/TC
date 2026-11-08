#pragma once

#include <glm/glm.hpp>
#include <string>

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
            const glm::vec3& color,
            const std::string& name = {}
        );

        glm::mat4 GetModelMatrix() const;

        PrimitiveType GetType() const;
        const std::string& GetName() const;
        glm::vec3 GetColor() const;
        glm::vec3 GetPosition() const;
        glm::vec3 GetRotation() const;
        glm::vec3 GetScale() const;

        void SetName(const std::string& name);
        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);
        void SetScale(const glm::vec3& scale);
        void SetColor(const glm::vec3& color);

        void Translate(const glm::vec3& offset);
        void Rotate(const glm::vec3& offset);
        void ScaleBy(float factor);

    private:
        PrimitiveType m_Type;
        std::string m_Name;
        glm::vec3 m_Position;
        glm::vec3 m_Rotation;
        glm::vec3 m_Scale;
        glm::vec3 m_Color;
    };
}
