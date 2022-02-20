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
    float xndc = (x + glm::linearRand(0.0f, 1.0f)) / imgWidth;
    float yndc = (y + glm::linearRand(0.0f, 1.0f)) / imgHeight;

    float xscreen = (xndc * 2) - 1;
    float yscreen = 1 - (yndc * 2);


    float aspectRatio = (float)imgWidth / imgHeight;
    float xcamera = xscreen * aspectRatio * glm::tan(glm::radians(fieldOfView));
    float ycamera = yscreen * glm::tan(glm::radians(fieldOfView));

    glm::vec3 rayDirection = glm::vec3(xcamera, ycamera, -1.0f);
    return glm::normalize(rayDirection);
}

RenderManager::RenderManager(RTCDevice* device, Camera camera, bool smoothShading, u_int32_t multisamplingIterations, u_int16_t maxRayDepth) :
    m_device(device), m_scene(nullptr),
    m_camera(camera), m_smoothShading(smoothShading),
    m_multisamplingIterations(multisamplingIterations), m_maxRayDepth(maxRayDepth),
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
            glm::vec3 pixelColour(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < m_multisamplingIterations; i++)
            {
                RTCIntersectContext context;
                rtcInitIntersectContext(&context);

                pixelColour += CastRay(m_camera.position, m_camera.getPixelRayDirection(x, y, imgWidth, imgHeight), m_camera.nearPlane, m_camera.farPlane, context, 0);
            }
            
            pixelColour.r = pixelColour.r / (float)m_multisamplingIterations;
            pixelColour.g = pixelColour.g / (float)m_multisamplingIterations;
            pixelColour.b = pixelColour.b / (float)m_multisamplingIterations;
            pixels.push_back(pixelColour);
        }
    }

    WriteToPPM(outputFileName, imgWidth, imgHeight, pixels);
}

glm::vec3 RenderManager::CastRay(glm::vec3 origin, glm::vec3 direction, float near, float far, RTCIntersectContext& context, u_int16_t rayDepth)
{
    RTCRayHit rayhit;
    {
        rayhit.ray.org_x = origin.x; rayhit.ray.org_y = origin.y; rayhit.ray.org_z = origin.z;
        rayhit.ray.dir_x = direction.x; rayhit.ray.dir_y = direction.y; rayhit.ray.dir_z = direction.z;
        rayhit.ray.tnear = near;
        rayhit.ray.tfar = far;
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }

    rtcIntersect1(m_scene, &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        MeshGeometry* hitMesh = m_meshObjects[rayhit.hit.geomID];
        MaterialProperties surfaceProperties = hitMesh->properties();

        glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);
        {
            hitPoint.x = rayhit.ray.org_x + rayhit.ray.dir_x * rayhit.ray.tfar;
            hitPoint.y = rayhit.ray.org_y + rayhit.ray.dir_y * rayhit.ray.tfar;
            hitPoint.z = rayhit.ray.org_z + rayhit.ray.dir_z * rayhit.ray.tfar;
        }
        glm::vec3 surfaceNormal(0.0f, 0.0f, 0.0f);
        {
            surfaceNormal.x = rayhit.hit.Ng_x;
            surfaceNormal.y = rayhit.hit.Ng_y;
            surfaceNormal.z = rayhit.hit.Ng_z;
            if (m_smoothShading)
            {
                float a, b, c;
                hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPoint, a, b, c);

                glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
                surfaceNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
                surfaceNormal = glm::normalize(surfaceNormal);
            }
        }
        glm::vec3 reflectionDirection = (-glm::normalize(direction)) * glm::angleAxis(glm::radians(180.0f), glm::normalize(surfaceNormal));
        {
            glm::vec3 randomDirection = glm::normalize(glm::sphericalRand(1.0f));
            if (randomDirection == -glm::normalize(surfaceNormal) || glm::dot(randomDirection, glm::normalize(surfaceNormal)) < 0.0f)
                randomDirection = -randomDirection;

            float angle = glm::acos(glm::dot(reflectionDirection, randomDirection));
            angle *= surfaceProperties.roughness;

            glm::vec3 perpendicular = glm::cross(reflectionDirection, randomDirection);
            reflectionDirection = reflectionDirection * glm::angleAxis(angle, glm::normalize(perpendicular));
        }

        double randChoice = glm::linearRand(0.0f, 1.0f);

        if (randChoice > surfaceProperties.glassiness || surfaceProperties.glassiness == 0.0f)
        {
            glm::vec3 diffuseColour(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < m_sceneLights.size(); i++)
            {
                diffuseColour += CalculateDiffuseColour(hitPoint, surfaceNormal, reflectionDirection, m_sceneLights[i], surfaceProperties, context);
            }

            glm::vec3 ambientColour(0.0f, 0.0f, 0.0f);
            if (rayDepth < m_maxRayDepth)
            {
                ambientColour = CastRay(hitPoint, reflectionDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth + 1);
                ambientColour *= surfaceProperties.lightReflection;
            }
            diffuseColour += ambientColour;

            return diffuseColour;
        }
        else
        {
            glm::vec3 glassyColour(0.0f, 0.0f, 0.0f); // The Reflection/Refraction Colour
            if (rayDepth < m_maxRayDepth)
            {
                float randChoice = glm::linearRand(0.0f, 1.0f);
                if (randChoice > surfaceProperties.translucency || surfaceProperties.translucency == 0.0f)
                {
                    glassyColour = CalculateReflectionColour(hitPoint, reflectionDirection, surfaceProperties, context, rayDepth + 1);
                }
                else
                {
                    glassyColour = CalculateRefractionColour(hitPoint, surfaceNormal, reflectionDirection, surfaceProperties, context, rayDepth);
                }
            }

            return glassyColour;
        }
    }

    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 RenderManager::CalculateDiffuseColour(glm::vec3 hitPoint, glm::vec3 surfaceNormal, glm::vec3 reflectionDirection, PointLight light, MaterialProperties surfaceProperties, RTCIntersectContext& context)
{
    glm::vec3 lightDirection = light.GetDirectionFromPoint(hitPoint);

    float facingRatio = glm::dot(glm::normalize(lightDirection), glm::normalize(surfaceNormal));
    if (facingRatio <= 0.0f)
        return glm::vec3(0.0f, 0.0f, 0.0f);

    RTCRay shadowray;
    {
        shadowray.org_x = hitPoint.x; shadowray.org_y = hitPoint.y; shadowray.org_z = hitPoint.z;
        shadowray.dir_x = lightDirection.x; shadowray.dir_y = lightDirection.y; shadowray.dir_z = lightDirection.z;
        shadowray.tnear = 0.01f;
        shadowray.tfar = 1.01f;
    }

    rtcOccluded1(m_scene, &context, &shadowray);
    if (shadowray.tfar >= 1.0f)
    {
        float lightDistance = light.GetDistanceFromPoint(hitPoint);
        float lightDim = 4 * glm::pi<float>() * glm::pow(lightDistance, 2.0f);
        glm::vec3 lightColour = (facingRatio * light.colour * light.intensity) / lightDim;

        float viewRatio = glm::dot(glm::normalize(reflectionDirection), glm::normalize(lightDirection));
        viewRatio = glm::clamp(viewRatio, 0.0f, 1.0f);

        glm::vec3 diffuseColour(0.0f, 0.0f, 0.0f); 
        {
            diffuseColour.r = surfaceProperties.albedoColour.r * lightColour.r;
            diffuseColour.g = surfaceProperties.albedoColour.g * lightColour.g;
            diffuseColour.b = surfaceProperties.albedoColour.b * lightColour.b;

            diffuseColour /= glm::pi<float>();
        }

        glm::vec3 glossColour = lightColour * glm::pow(viewRatio, surfaceProperties.glossyFalloff) * surfaceProperties.glossiness;

        return diffuseColour + glossColour;
    }

    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 RenderManager::CalculateReflectionColour(glm::vec3 hitPoint, glm::vec3 reflectionDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth)
{
    glm::vec3 reflectionColour = CastRay(hitPoint, reflectionDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth);
    {
        reflectionColour.r *= surfaceProperties.albedoColour.r;
        reflectionColour.g *= surfaceProperties.albedoColour.g;
        reflectionColour.b *= surfaceProperties.albedoColour.b;
    }

    return reflectionColour;
}

glm::vec3 RenderManager::CalculateRefractionColour(glm::vec3 hitPoint, glm::vec3 surfaceNormal, glm::vec3 reflectionDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth)
{
    float incidenceAngle = glm::acos(glm::dot(glm::normalize(surfaceNormal), glm::normalize(reflectionDirection)));
    float refractionAngle = glm::asin((1/surfaceProperties.refractiveIndex) * glm::sin(incidenceAngle));
    glm::vec3 perpendicular = glm::normalize(glm::cross(glm::normalize(reflectionDirection), glm::normalize(surfaceNormal)));
    glm::vec3 refractionDirection = -surfaceNormal * glm::angleAxis(refractionAngle, perpendicular);

    RTCRayHit refractionRay;
    {
        refractionRay.ray.org_x = hitPoint.x; refractionRay.ray.org_y = hitPoint.y; refractionRay.ray.org_z = hitPoint.z;
        refractionRay.ray.dir_x = refractionDirection.x; refractionRay.ray.dir_y = refractionDirection.y; refractionRay.ray.dir_z = refractionDirection.z;
        refractionRay.ray.tnear = 0.01f;
        refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
    }
    rtcIntersect1(m_scene, &context, &refractionRay);

    glm::vec3 refractionColour(0.0f, 0.0f, 0.0f);
    int internalReflections = 0;
    while (true)
    {
        glm::vec3 interiorNormal;
        {
            interiorNormal.x = -refractionRay.hit.Ng_x; interiorNormal.y = -refractionRay.hit.Ng_y; interiorNormal.z = -refractionRay.hit.Ng_z;
        }
        {
            hitPoint.x = refractionRay.ray.org_x + refractionRay.ray.dir_x * refractionRay.ray.tfar;
            hitPoint.y = refractionRay.ray.org_y + refractionRay.ray.dir_y * refractionRay.ray.tfar;
            hitPoint.z = refractionRay.ray.org_z + refractionRay.ray.dir_z * refractionRay.ray.tfar;
        }
        float interiorAngleSin = glm::sin(glm::dot(glm::normalize(-interiorNormal), glm::normalize(refractionDirection)));
        float exitAngleSin = surfaceProperties.refractiveIndex * interiorAngleSin;

        // Send Ray out to World
        if (exitAngleSin <= 1.0f)
        {
            perpendicular = glm::normalize(glm::cross(glm::normalize(refractionDirection), glm::normalize(-interiorNormal)));
            glm::vec3 exitDirection = -interiorNormal * glm::quat(glm::asin(exitAngleSin), -interiorNormal);

            refractionColour = CastRay(hitPoint, exitDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth + internalReflections);
            break;
        }
        // Internally Reflect Ray
        else if (rayDepth + internalReflections < m_maxRayDepth)
        {
            internalReflections++;
            refractionDirection = -refractionDirection * glm::angleAxis(glm::radians(180.0f), interiorNormal);

            {
                refractionRay.ray.org_x = hitPoint.x; refractionRay.ray.org_y = hitPoint.y; refractionRay.ray.org_z = hitPoint.z;
                refractionRay.ray.dir_x = refractionDirection.x; refractionRay.ray.dir_y = refractionDirection.y; refractionRay.ray.dir_z = refractionDirection.z;
                refractionRay.ray.tnear = 0.01f;
                refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
            }

            rtcIntersect1(m_scene, &context, &refractionRay);
        }
        else
            break;
    }

    return refractionColour;
}