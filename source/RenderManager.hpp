#include <embree3/rtcore.h>

#include <vector>
#include <glm/glm.hpp>

#include "MeshGeometry.hpp"

class RenderManager
{
public:
    RenderManager(RTCDevice* device);

private:
    RTCDevice* m_device;
    RTCScene m_scene;

    std::vector<MeshGeometry*> m_meshObjects;
    MeshProperties getMeshGeometryProperties(int meshGeometryID) { return m_meshObjects[meshGeometryID]->properties(); }

public:
    void AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position);
};