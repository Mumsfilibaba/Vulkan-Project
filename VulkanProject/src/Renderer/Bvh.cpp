#include "Bvh.h"
#include "Scene.h"

FBvhScene::FBvhScene()
{
}

void FBvhScene::Build(const FScene& Scene)
{
    for (uint32_t QuadIndex = 0; QuadIndex < Scene.m_Quads.size(); QuadIndex++)
    {
        glm::vec3 AABBMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 AABBMax = glm::vec3(std::numeric_limits<float>::lowest());

        const FQuad& Quad = Scene.m_Quads[QuadIndex];
        const glm::vec3 p0 = Quad.Position;
        const glm::vec3 p1 = Quad.Position + Quad.Edge0;
        const glm::vec3 p2 = Quad.Position + Quad.Edge1;
        const glm::vec3 p3 = Quad.Position + Quad.Edge0 + Quad.Edge1;

        AABBMin.x = std::min({ AABBMin.x, p0.x, p1.x, p2.x, p3.x });
        AABBMin.y = std::min({ AABBMin.y, p0.y, p1.y, p2.y, p3.y });
        AABBMin.z = std::min({ AABBMin.z, p0.z, p1.z, p2.z, p3.z });

        AABBMax.x = std::max({ AABBMax.x, p0.x, p1.x, p2.x, p3.x });
        AABBMax.y = std::max({ AABBMax.y, p0.y, p1.y, p2.y, p3.y });
        AABBMax.z = std::max({ AABBMax.z, p0.z, p1.z, p2.z, p3.z });
        
        m_Nodes.push_back(
        {
            glm::vec4(AABBMin, 0.0f),
            glm::vec4(AABBMax, 0.0f),
            static_cast<uint32_t>(EObjectType::Quad),
            QuadIndex,
            // Padding
            0, 0
        });
    }

    for (uint32_t SphereIndex = 0; SphereIndex < Scene.m_Spheres.size(); SphereIndex++)
    {
        glm::vec3 AABBMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 AABBMax = glm::vec3(std::numeric_limits<float>::lowest());

        const FSphere& Sphere = Scene.m_Spheres[SphereIndex];
        const glm::vec3 min = Sphere.Position - glm::vec3(Sphere.Radius);
        const glm::vec3 max = Sphere.Position + glm::vec3(Sphere.Radius);
        
        AABBMin.x = std::min({ AABBMin.x, min.x, max.x });
        AABBMin.y = std::min({ AABBMin.y, min.y, max.y });
        AABBMin.z = std::min({ AABBMin.z, min.z, max.z });

        AABBMax.x = std::max({ AABBMax.x, min.x, max.x });
        AABBMax.y = std::max({ AABBMax.y, min.y, max.y });
        AABBMax.z = std::max({ AABBMax.z, min.z, max.z });

        m_Nodes.push_back(
        {
            glm::vec4(AABBMin, 0.0f),
            glm::vec4(AABBMax, 0.0f),
            static_cast<uint32_t>(EObjectType::Sphere),
            SphereIndex,
            // Padding
            0, 0
        });
    }
}
