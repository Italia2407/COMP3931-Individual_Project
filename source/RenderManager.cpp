#include "RenderManager.hpp"

#include "IOManagers/PPMWriter.hpp"

#include <iostream>

RenderManager::RenderManager(RTCDevice* device, float borderL, float borderR, float borderT, float borderB) :
    m_device(device), m_scene(nullptr), m_borderL(m_borderL), m_borderR(borderR), m_borderT(borderT), m_borderB(borderB)
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

void RenderManager::RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight)
{
    rtcCommitScene(m_scene);

    std::vector<glm::vec3> pixels = std::vector<glm::vec3>();

    for (int y = 0; y < imgHeight; y++)
    {
        float ypos = m_borderT + ((float)y / imgHeight) * (m_borderB - m_borderT);
        for (int x = 0; x < imgWidth; x++)
        {
            float xpos = m_borderL + ((float)x / imgWidth) * (m_borderR - m_borderL);

            RTCRayHit rayhit;
            rayhit.ray.org_x = xpos; rayhit.ray.org_y = ypos; rayhit.ray.org_z = -1.0f;
            rayhit.ray.dir_x = 0.0f; rayhit.ray.dir_y = 0.0f; rayhit.ray.dir_z = 1.0f;
            rayhit.ray.tnear = 0.01f;
            rayhit.ray.tfar = 100.0f;
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);
            rtcIntersect1(m_scene, &context, &rayhit);

            if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
            {
                std::cout << getMeshGeometryProperties(rayhit.hit.geomID).asciiFace;
                pixels.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
            }
            else
            {
                std::cout << ',';
                pixels.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
            }
        }

        std::cout << std::endl;
    }

    WriteToPPM(outputFileName, imgWidth, imgHeight, pixels);
}