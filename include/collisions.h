#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>

// Teste de interseção entre esfera e plano
bool TestIntersectionSpherePlane(glm::vec3 sphereCenter, float sphereRadius, float groundY);

// Teste de intersecção entre uma Esfera e uma Caixa (AABB)
bool TestIntersectionSphereAABB(glm::vec3 sphereCenter, float sphereRadius, glm::vec3 aabbMin, glm::vec3 aabbMax);

#endif