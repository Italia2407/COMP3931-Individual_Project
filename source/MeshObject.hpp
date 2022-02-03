#include <embree3/rtcore.h>

#include <glm/glm.h>
#include <vector>

struct MeshProperties
{
public:
    MeshProperties(char AsciiFace);

    char asciiFace;
};

class MeshObject
{
public:
    MeshObject(RTCDevice* device, RTCScene* scene, MeshProperties properties);

    static std::vector<MeshObject> meshObjects;
    static MeshProperties getProperties(int meshID) { return MeshObject::meshObjects[meshID]; }

private:
    RTCDevice* m_device;
    RTCScene* m_scene

    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec3<unsigned int>> m_faces;

    MeshProperties m_properties;

public:
    const std::vector<glm::vec3>& vertices() { return m_vertices; }
    const std::vector<glm::vec3<unsigned int>>& faces() { return m_faces; }

    const MeshProperties& properties() { return m_properties; }

public:
    bool LoadFromOBJ(std::string fileName);
    
    bool BindToScene(glm::vec3 position);
};