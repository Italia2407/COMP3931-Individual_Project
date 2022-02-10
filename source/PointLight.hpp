#include <glm/glm.hpp>

struct PointLight
{
public:
    PointLight(glm::vec3 position);
    PointLight(glm::vec3 position, glm::vec3 colour);

    glm::vec3 position;
    glm::vec3 colour;
};