#include "entity.h"

#include <glm/gtx/quaternion.hpp> // quatToMat4

void Entity::init(Entity* entity)
{
    // zero out
    *entity = {};
    // init transform
    entity->pos = glm::vec3(0.0);
    entity->rot = QUAT_IDENTITY;
    entity->sca = glm::vec3(1.0);

    // init camera params
    entity->fovDegrees = 45.0f;
    entity->aspect     = 1.0f; // TODO get from window in g_ctx
    entity->nearPlane  = 0.1f;
    entity->farPlane   = 1000.0f;
}

glm::mat4 Entity::modelMatrix(Entity* entity)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, entity->pos);
    M           = M * glm::toMat4(entity->rot);
    M           = glm::scale(M, entity->sca);
    return M;
}

glm::mat4 Entity::viewMatrix(Entity* entity)
{
    // return glm::inverse(modelMatrix(entity));

    // optimized version for camera only (doesn't take scale into account)
    glm::mat4 invT = glm::translate(MAT_IDENTITY, -entity->pos);
    glm::mat4 invR = glm::toMat4(glm::conjugate(entity->rot));
    return invR * invT;
}

glm::mat4 Entity::projectionMatrix(Entity* entity)
{
    return glm::perspective(glm::radians(entity->fovDegrees), entity->aspect,
                            entity->nearPlane, entity->farPlane);
}

void Entity::rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = entity->rot * glm::angleAxis(deg, glm::normalize(axis));
}

void Entity::rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = glm::angleAxis(deg, glm::normalize(axis)) * entity->rot;
}