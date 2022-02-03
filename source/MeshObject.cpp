#include "MeshObject.hpp"

MeshObject::MeshObject(RTCDevice* device, RTCScene* scene) :
    m_device(device), m_scene(scene), m_vertices(std::vector<glm::vec3>()), m_faces(std::vector<glm::vec3<int>>()) {}
