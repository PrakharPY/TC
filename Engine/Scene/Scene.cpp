#include "Scene.h"

namespace TC
{
    Scene::Scene()
    {
        // Ground plane
        m_Objects.emplace_back(
            PrimitiveType::Plane,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(20.0f, 1.0f, 20.0f),
            glm::vec3(0.32f, 0.34f, 0.38f)
        );

        // First cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(-1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f),
            glm::vec3(0.95f, 0.25f, 0.20f)
        );

        // Second cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f, 30.0f, 0.0f),
            glm::vec3(1.0f),
            glm::vec3(0.20f, 0.55f, 0.95f)
        );

        // Third, smaller cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(0.0f, 0.5f, -2.0f),
            glm::vec3(0.0f),
            glm::vec3(0.65f),
            glm::vec3(0.25f, 0.85f, 0.40f)
        );
    }

    void Scene::AddObject(const SceneObject& object)
    {
        m_Objects.push_back(object);
    }

    const std::vector<SceneObject>& Scene::GetObjects() const
    {
        return m_Objects;
    }
}