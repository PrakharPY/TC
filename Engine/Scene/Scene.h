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

        // Returns the index of the newly created object, or -1 on failure.
        int CreateObject(PrimitiveType type, const glm::vec3& position);
        int DuplicateObject(int index);
        bool RemoveObject(int index);
        void SetObjects(const std::vector<SceneObject>& objects);

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
