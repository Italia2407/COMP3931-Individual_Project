#include "RenderManager.hpp"

#include "../IOManagers/PPMWriter.hpp"

#include <iostream>
#include <limits>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/quaternion.hpp>

Camera::Camera(glm::vec3 position, float fov, float np, float fp) :
        position(position), fieldOfView(fov), nearPlane(np), farPlane(fp) {}

glm::vec3 Camera::getPixelRayDirection(int x, int y, u_int16_t imgWidth, u_int16_t imgHeight)
{
    float xndc = (x + 0.5f) / imgWidth;
    float yndc = (y + 0.5f) / imgHeight;

    float xscreen = (xndc * 2) - 1;
    float yscreen = 1 - (yndc * 2);


    float aspectRatio = (float)imgWidth / imgHeight;
    float xcamera = xscreen * aspectRatio * glm::tan(glm::radians(fieldOfView));
    float ycamera = yscreen * glm::tan(glm::radians(fieldOfView));

    glm::vec3 rayDirection = glm::vec3(xcamera, ycamera, -1.0f);
    return glm::normalize(rayDirection);
}

RenderManager::RenderManager(RTCDevice* device, Camera camera, bool smoothShading, u_int16_t maxRayDepth) :
    m_device(device), m_scene(nullptr),
    m_camera(camera), m_smoothShading(smoothShading),
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

    unsigned int* faces = (unsigned int*)rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned int), meshGeometry->faceVIDs().size());
    for (int i = 0; i < meshGeometry->faceVIDs().size(); i++)
    {
        faces[i*3 + 0] = meshGeometry->faceVIDs()[i].x;
        faces[i*3 + 1] = meshGeometry->faceVIDs()[i].y;
        faces[i*3 + 2] = meshGeometry->faceVIDs()[i].z;
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
        for (int x = 0; x < imgWidth; x++)
        {
            u_int16_t rayDepth = 0;   // Ray Depth Initialised to 0
            glm::vec3 pixelColour = TraceRay(m_camera.position, m_camera.getPixelRayDirection(x, y, imgWidth, imgHeight), m_camera.nearPlane, m_camera.farPlane, rayDepth);

            pixels.push_back(pixelColour);
        }
    }

    WriteToPPM(outputFileName, imgWidth, imgHeight, pixels);
}

glm::vec3 RenderManager::TraceRay(glm::vec3 origin, glm::vec3 direction, float near, float far, u_int16_t& rayDepth)
{
    RTCRayHit rayhit;

    rayhit.ray.org_x = origin.x; rayhit.ray.org_y = origin.y; rayhit.ray.org_z = origin.z;
    rayhit.ray.dir_x = direction.x; rayhit.ray.dir_y = direction.y; rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = near;
    rayhit.ray.tfar = far;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    rtcIntersect1(m_scene, &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        rayDepth++;

        MeshGeometry* hitMesh = m_meshObjects[rayhit.hit.geomID];
        MeshProperties surfaceProperties = hitMesh->properties();

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
            if (m_smoothShading)
            {
                float a, b, c;

                //std::cout << "Seg Here?\n";
                hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPosition, surfaceNormal, a, b, c);
                //std::cout << "Seg Not Here.\n";

                glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
                //std::cout << "Seg Here!\n";
                //surfaceNormal = (hitMesh->normals()[hitFace.x] * a) + (hitMesh->normals()[hitFace.y] * b) + (hitMesh->normals()[hitFace.z] * c);
                surfaceNormal = (hitMesh->normals()[hitFace.x] * a) + (hitMesh->normals()[hitFace.y] * b) + (hitMesh->normals()[hitFace.z] * c);
                //std::cout << "Then Seg Here!\n";
            }
        }
        glm::vec3 reflectionDirection = glm::angleAxis(glm::radians(180.0f), surfaceNormal) * -direction;

        glm::vec3 surfaceColour = CastShadowRays(hitPosition, surfaceNormal, reflectionDirection, surfaceProperties);
        glm::vec3 reflectionColour(0.0f, 0.0f, 0.0f);
        glm::vec3 refractionColour(0.0f, 0.0f, 0.0f);

        if (rayDepth < m_maxRayDepth)
        {
            // Calcualte Reflection Ray Colour
            {
                reflectionColour = TraceRay(hitPosition, reflectionDirection, 0.0f, std::numeric_limits<float>::infinity(), rayDepth);
            }

            // Calculate Refraction Ray Colour
            {

            }
        }

        glm::vec3 returnColour = (surfaceProperties.reflectivity * reflectionColour) + ((1-surfaceProperties.reflectivity) * surfaceColour);
        returnColour = (surfaceProperties.translucency * refractionColour) + ((1-surfaceProperties.translucency) * returnColour);

        return returnColour;
    }
    else
    {
        if (rayDepth == 0)
            return glm::vec3(0.4f, 0.5f, 0.65f);
        
        else
            return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

glm::vec3 RenderManager::CastShadowRays(glm::vec3 hitPosition, glm::vec3 surfaceNormal, glm::vec3 reflectionDirection, MeshProperties surfaceProperties)
{
    glm::vec3 resultDiffuseColour = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 resultGlossColour = glm::vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < m_sceneLights.size(); i++)
    {
        glm::vec3 lightDirection = m_sceneLights[i].GetDirectionFromPoint(hitPosition);

        float facingRatio = glm::dot(glm::normalize(lightDirection), glm::normalize(surfaceNormal));
        if (facingRatio <= 0.0f)
            continue;

        RTCRay shadowray;
        
        shadowray.org_x = hitPosition.x; shadowray.org_y = hitPosition.y; shadowray.org_z = hitPosition.z;
        shadowray.dir_x = lightDirection.x; shadowray.dir_y = lightDirection.y; shadowray.dir_z = lightDirection.z;
        shadowray.tnear = 0.0f;
        shadowray.tfar = 1.0f;

        glm::vec3 diffuseColour(0.0f, 0.0f, 0.0f);
        glm::vec3 glossColour(0.0f, 0.0f, 0.0f);

        if (shadowray.tfar >= 1.0f)
        {
            float lightDistance = m_sceneLights[i].GetDistanceFromPoint(hitPosition);
            float surfaceArea = 4 * glm::pi<float>() * glm::pow(lightDistance, 2.0f);
            glm::vec3 lightColour = (m_sceneLights[i].colour * m_sceneLights[i].intensity) / surfaceArea;

            // Calculate Diffuse Colour
            {
                diffuseColour.r = surfaceProperties.albedo.r * (lightColour.r * facingRatio);
                diffuseColour.g = surfaceProperties.albedo.g * (lightColour.g * facingRatio);
                diffuseColour.b = surfaceProperties.albedo.b * (lightColour.b * facingRatio);

                diffuseColour /= glm::pi<float>();
            }

            // Calculate Gloss Colour
            {
                float viewRatio = glm::dot(glm::normalize(reflectionDirection), glm::normalize(lightDirection));
                viewRatio = glm::clamp(viewRatio, 0.0f, 1.0f);
                glossColour = lightColour * glm::pow(viewRatio, surfaceProperties.glossFalloff);
            }
        }

        resultDiffuseColour += diffuseColour;
        resultGlossColour += glossColour;
    }

    return resultDiffuseColour + (resultGlossColour * surfaceProperties.glossiness);
}