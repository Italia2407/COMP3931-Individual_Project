#include <embree3/rtcore.h>

#include <glm/glm.h>
#include <vector>

struct MeshObject
{
public:
    MeshObject(RTCDevice* device, RTCScene* scene);

private:
    RTCDevice* m_device;
    RTCScene* m_scene

    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec3<int>> m_faces;

public:
    const std::vector<glm::vec3>& vertices() { return m_vertices; }
    const std::vector<glm::vec3<int>>& faces() { return m_faces; }
};