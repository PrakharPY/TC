#pragma once

#include "SceneObject.h"

#include <vector>
#include <glm/glm.hpp>

namespace TC
{
    class Scene
    {
    public:
        Scene();

        void AddObject(const SceneObject& object);

        const std::vector<SceneObject>& GetObjects() const;

        SceneObject* GetObject(int index);

        int PickObject(
            const glm::vec3& rayOrigin,
            const glm::vec3& rayDirection
        ) const;

    private:
        std::vector<SceneObject> m_Objects;
    };
}