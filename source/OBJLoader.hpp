#include "MeshObject.hpp"

#include <glm/glm.h>

#include <string>
#include <vector>

class OBJLoader
{
public:
    static bool LoadObject(std::string fileName, MeshObject* mesh);
};