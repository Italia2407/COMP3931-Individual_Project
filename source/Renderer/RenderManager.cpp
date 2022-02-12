#include "RenderManager.hpp"

#include "../IOManagers/PPMWriter.hpp"

#include <iostream>
#include <limits>
#include <glm/gtc/constants.hpp>

RenderManager::RenderManager(RTCDevice* device, float borderL, float borderR, float borderT, float borderB, u_int16_t maxRayDepth) :
    m_device(device), m_scene(nullptr),
    m_borderL(borderL), m_borderR(borderR), m_borderT(borderT), m_borderB(borderB), m_maxRayDepth(maxRayDepth),
    m_meshObjects(std::vector<MeshGeometry*>()), m_sceneLights(std::vector<PointLight>())
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

void RenderManager::AddLight(glm::vec3 position, glm::vec3 colour, float intensity)
{
    PointLight sceneLight = PointLight(position, colour, intensity);
    m_sceneLights.push_back(sceneLight);
}

void RenderManager::RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight)
{
    rtcCommitScene(m_scene);

    std::vector<glm::vec3> pixels = std::vector<glm::vec3>();

    for (int y = 0; y < imgHeight; y++)
    {
        float ypos = m_borderT + ((y / (float)imgHeight) * (m_borderB - m_borderT));
        for (int x = 0; x < imgWidth; x++)
        {
            float xpos = m_borderL + ((x / (float)imgWidth) * (m_borderR - m_borderL));

            u_int16_t rayDepth = 0;   // Ray Depth Initialised to 0
            glm::vec3 pixelColour = TraceRay(glm::vec3(xpos, ypos, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f), rayDepth);

            pixels.push_back(pixelColour);
        }
    }

    WriteToPPM(outputFileName, imgWidth, imgHeight, pixels);
}

glm::vec3 RenderManager::TraceRay(glm::vec3 origin, glm::vec3 direction, u_int16_t& rayDepth)
{
    RTCRayHit rayhit;

    rayhit.ray.org_x = origin.x; rayhit.ray.org_y = origin.y; rayhit.ray.org_z = origin.z;
    rayhit.ray.dir_x = direction.x; rayhit.ray.dir_y = direction.y; rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    rtcIntersect1(m_scene, &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        rayDepth++;

        glm::vec3 hitPosition;
        {
            hitPosition.x = rayhit.ray.org_x + rayhit.ray.dir_x * rayhit.ray.tfar;
            hitPosition.y = rayhit.ray.org_y + rayhit.ray.dir_y * rayhit.ray.tfar;
            hitPosition.z = rayhit.ray.org_z + rayhit.ray.dir_z * rayhit.ray.tfar;
        }
        glm::vec3 surfaceNormal;
        {
            surfaceNormal.x = rayhit.hit.Ng_x;
            surfaceNormal.y = rayhit.hit.Ng_y;
            surfaceNormal.z = rayhit.hit.Ng_z;
        }
        glm::vec3 pointColour = getMeshGeometryProperties(rayhit.hit.geomID).surfaceColour;

        glm::vec3 returnColour = CastShadowRays(hitPosition, surfaceNormal, pointColour);

        if (rayDepth < m_maxRayDepth)
        {

        }

        return returnColour;
    }
    else
        return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 RenderManager::CastShadowRays(glm::vec3 origin, glm::vec3 normal, glm::vec3 albedo)
{
    glm::vec3 finalPointColour = glm::vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < m_sceneLights.size(); i++)
    {
        glm::vec3 lightDirection = m_sceneLights[i].GetDirectionFromPoint(origin);

        float facingRatio = glm::dot(glm::normalize(lightDirection), glm::normalize(normal));
        if (facingRatio <= 0.0f)
            continue;

        RTCRay shadowray;
        
        shadowray.org_x = origin.x; shadowray.org_y = origin.y; shadowray.org_z = origin.z;
        shadowray.dir_x = lightDirection.x; shadowray.dir_y = lightDirection.y; shadowray.dir_z = lightDirection.z;
        shadowray.tnear = 0.0f;
        shadowray.tfar = 1.0f;

        if (shadowray.tfar >= 1.0f)
        {
            glm::vec3 lightColour = m_sceneLights[i].colour * m_sceneLights[i].intensity;
            float lightDistance = m_sceneLights[i].GetDistanceFromPoint(origin);
            float surfaceArea = 4 * glm::pi<float>() * glm::pow(lightDistance, 2.0f);

            {
                glm::vec3 pointColour;
                
                pointColour.r = ((lightColour.r / glm::pi<float>()) * facingRatio) / surfaceArea;
                pointColour.g = ((lightColour.g / glm::pi<float>()) * facingRatio) / surfaceArea;
                pointColour.b = ((lightColour.b / glm::pi<float>()) * facingRatio) / surfaceArea;

                finalPointColour += pointColour;
            }
        }
    }

    return finalPointColour;
}