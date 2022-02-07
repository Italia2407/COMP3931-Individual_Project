#include "RenderManager.hpp"

RenderManager::RenderManager(RTCDevice* device) :
    m_device(device), m_scene(nullptr)
{
    if (m_device != nullptr)
        m_scene = rtcNewScene(*device);
}

void RenderManager::AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position)
{
    RTCGeometry geometry = rtcNewGeometry(*m_device, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = (float*)rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), meshGeometry->vertices().size());
    for (int i = 0; i < meshGeometry->vertices().size(); i++)
    {
        vertices[i*3 + 0] = meshGeometry->vertices()[i].x + position.x;
        vertices[i*3 + 1] = meshGeometry->vertices()[i].y + position.y;
        vertices[i*3 + 2] = meshGeometry->vertices()[i].z + position.z;
    }

    unsigned int* faces = (unsigned int*)rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned int), meshGeometry->faces().size());
    for (int i = 0; i < meshGeometry->faces().size(); i++)
    {
        faces[i*3 + 0] = meshGeometry->faces()[i].x;
        faces[i*3 + 1] = meshGeometry->faces()[i].y;
        faces[i*3 + 2] = meshGeometry->faces()[i].z;
    }

    rtcCommitGeometry(geometry);
    rtcAttachGeometry(m_scene, geometry);
    rtcReleaseGeometry(geometry);

    m_meshObjects.push_back(meshGeometry);
}