#include "collisions.h"
#include <algorithm>

//Teste de interseção entre esfera e plano.
bool TestIntersectionSpherePlane(glm::vec3 sphereCenter, float sphereRadius, float groundY) 
{
    return (sphereCenter.y - sphereRadius) <= groundY;
}

// Teste de intersecção entre uma Esfera e uma Caixa (AABB)
bool TestIntersectionSphereAABB(glm::vec3 sphereCenter, float sphereRadius, glm::vec3 aabbMin, glm::vec3 aabbMax) 
{
    float px = std::max(aabbMin.x, std::min(sphereCenter.x, aabbMax.x));
    float py = std::max(aabbMin.y, std::min(sphereCenter.y, aabbMax.y));
    float pz = std::max(aabbMin.z, std::min(sphereCenter.z, aabbMax.z));

    float distanceSquared = (px - sphereCenter.x) * (px - sphereCenter.x) +
                            (py - sphereCenter.y) * (py - sphereCenter.y) +
                            (pz - sphereCenter.z) * (pz - sphereCenter.z);

    return distanceSquared <= (sphereRadius * sphereRadius);
}
