#include "RenderManager.hpp"

#include "../IOManagers/PPMWriter.hpp"

#include <iostream>
#include <limits>
#include <chrono>
#include "../../cdalitz-kdtree-cpp/kdtree.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/quaternion.hpp>

static const int emittedPhotons = 250000;
static int photonsEmitted = 0;

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
    m_device(device), m_scene(nullptr), m_photonMapper(nullptr),
    m_camera(camera), m_smoothShading(smoothShading),
    m_multisamplingIterations(multisamplingIterations), m_maxRayDepth(maxRayDepth),
    m_meshObjects(std::vector<MeshGeometry*>()), m_sceneLights(std::vector<PointLight>())
{
    if (m_device != nullptr)
        m_scene = rtcNewScene(*device);

    m_photonMapper = new PhotonMapper(&m_meshObjects, true, emittedPhotons, 8);
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
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    std::vector<glm::vec3> pixels = std::vector<glm::vec3>(imgWidth * imgHeight);

    auto start_r = std::chrono::steady_clock::now();
    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            glm::vec3 pixelColour(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < m_multisamplingIterations; i++)
            {
                //std::cout << "Pixel (" << x << ", " << y << "): Iteration #" << i << std::endl;

                CastRay(m_camera.position, m_camera.getPixelRayDirection(x, y, imgWidth, imgHeight), m_camera.nearPlane, m_camera.farPlane, context, 0, glm::uvec2(x, y));
            }
        }
    }
    auto end_r = std::chrono::steady_clock::now();
    auto millisecondDuration_r = std::chrono::duration_cast<std::chrono::milliseconds>(end_r - start_r).count();
    std::cout << "Seconds Elapsed for Ray Mapping: " << millisecondDuration_r << "ms" << std::endl;

    for (int i = 0; i < 16; i++)
    {
        auto start_p = std::chrono::steady_clock::now();

        for (PointLight light : m_sceneLights)
        {
            photonsEmitted += m_photonMapper->GeneratePhotons(light, m_scene);
        }
        auto end_p = std::chrono::steady_clock::now();
        auto millisecondDuration_p = std::chrono::duration_cast<std::chrono::milliseconds>(end_p - start_p).count();
        std::cout << "Seconds Elapsed for Photon Mapping Stage #" << i << ": " << millisecondDuration_p << "ms" << std::endl;

        auto start_i = std::chrono::steady_clock::now();

        for (RayHitPoint* r : m_hitPoints)
        {
            int oldPhotonCount = r->photonCount;
            float oldPhotonRadius = r->photonRadius;

            // TODO Add photon flux to hitpoints
            CalculateCausticColour(r, context, i);

            int addedPhotons = r->photonCount - oldPhotonCount;
            int keptPhotons = oldPhotonCount + glm::round(addedPhotons * m_alphaReduction);

            // Reduce Radius
            float newRadius = r->photonRadius * glm::sqrt((float)keptPhotons / r->photonCount);
            r->photonRadius = newRadius;

            // Flux Correction
            glm::vec3 newFlux = r->accumulatedFlux * ((float)keptPhotons / r->photonCount);
            r->accumulatedFlux = newFlux;

            r->photonCount = keptPhotons;
        }
        m_photonMapper->ClearPhotons();

        // TODO Add Hitpoints to Pixels
        for (RayHitPoint* r : m_hitPoints)
        {
            glm::vec3 finalColour = r->accumulatedFlux / ((glm::pi<float>() * glm::pow(r->photonRadius, 2.0f)) * photonsEmitted);
            finalColour /= m_multisamplingIterations; // Each point contributes to a fraction depending on multisampling iterations

            pixels[r->imageLocation.x + (r->imageLocation.y * imgWidth)] += finalColour;
            // std::cout << "Pixel: " << r->accumulatedFlux.r << std::endl;
            // std::cout << "Radius: " << r->photonRadius << std::endl;
        }

        auto end_i = std::chrono::steady_clock::now();
        auto millisecondDuration_i = std::chrono::duration_cast<std::chrono::milliseconds>(end_i - start_i).count();
        std::cout << "Seconds Elapsed for Rendering Stage #" << i << ": " << millisecondDuration_i << "ms" << std::endl;

        std::string outputName(outputFileName);
        WriteToPPM(outputName.append(std::to_string(i)), imgWidth, imgHeight, pixels);
    }
}

glm::vec3 RenderManager::CastRay(glm::vec3 origin, glm::vec3 direction, float near, float far, RTCIntersectContext& context, u_int16_t rayDepth, glm::uvec2 imageLocation)
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
            //std::cout << "NormX: " << surfaceNormal.x << ", NormY: " << surfaceNormal.y << ", NormZ: " << surfaceNormal.z << std::endl;
            if (m_smoothShading)
            {
                float a, b, c;
                hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPoint, a, b, c);

                glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
                surfaceNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
                surfaceNormal = glm::normalize(surfaceNormal);
            }
            //std::cout << "SmoothX: " << surfaceNormal.x << ", SmoothY: " << surfaceNormal.y << ", SmoothZ: " << surfaceNormal.z << std::endl;
            //std::cout << std::endl;
        }
        glm::vec3 reflectionDirection = direction - (2 * glm::dot(glm::normalize(direction), glm::normalize(surfaceNormal)) * surfaceNormal);
        glm::vec3 incidentDirection = glm::vec3(0.0f, 0.0f, 0.0f);
        {
            glm::vec3 randomDirection = glm::normalize(glm::sphericalRand(1.0f));
            if (randomDirection == -glm::normalize(surfaceNormal) || glm::dot(randomDirection, glm::normalize(surfaceNormal)) < 0.0f)
                randomDirection = -randomDirection;

            float angle = glm::acos(glm::dot(reflectionDirection, randomDirection));
            angle *= surfaceProperties.roughness;

            glm::vec3 perpendicular = glm::cross(reflectionDirection, randomDirection);
            reflectionDirection = reflectionDirection * glm::angleAxis(angle, glm::normalize(perpendicular));

            incidentDirection = reflectionDirection - (2 * glm::dot(glm::normalize(reflectionDirection), glm::normalize(-surfaceNormal)) * (-surfaceNormal));
        }

        double randChoice = glm::linearRand(0.0f, 1.0f);

        if (randChoice > surfaceProperties.glassiness || surfaceProperties.glassiness == 0.0f)
        {
            glm::vec3 diffuseColour(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < m_sceneLights.size(); i++)
            {
                diffuseColour += CalculateDiffuseColour(hitPoint, surfaceNormal, reflectionDirection, m_sceneLights[i], surfaceProperties, context);
                //diffuseColour += CalculateCausticColour(hitPoint, surfaceNormal, reflectionDirection, m_sceneLights[i], surfaceProperties, context);
            }

            // glm::vec3 ambientColour(0.0f, 0.0f, 0.0f);
            // if (rayDepth < m_maxRayDepth)
            // {
            //     ambientColour = CastRay(hitPoint, reflectionDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth + 1);
            //     ambientColour *= surfaceProperties.lightReflection;
            // }
            // diffuseColour += ambientColour;

            RayHitPoint* rayHitPoint = new RayHitPoint();
            {
                rayHitPoint->position = hitPoint;
                rayHitPoint->imageLocation = imageLocation;

                rayHitPoint->surfaceNormal = surfaceNormal;
                rayHitPoint->incidentDirection = incidentDirection;
                rayHitPoint->surfaceProperties = surfaceProperties;

                rayHitPoint->photonCount = 0;
                rayHitPoint->photonRadius = 0.1f;
                rayHitPoint->accumulatedFlux = glm::vec3(0.0f, 0.0f, 0.0f);
            }
            m_hitPoints.push_back(rayHitPoint);

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
                    glassyColour = CalculateReflectionColour(hitPoint, reflectionDirection, surfaceProperties, context, rayDepth + 1, imageLocation);
                }
                else
                {
                    glassyColour = CalculateRefractionColour(hitPoint, surfaceNormal, incidentDirection, surfaceProperties, context, rayDepth, imageLocation);
                }
            }

            return glassyColour;
        }
    }

    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 RenderManager::CalculateCausticColour(RayHitPoint* rayHitPoint, RTCIntersectContext& context, int iterationNum)
{
    float kValue = 1.0f;

    glm::vec3 causticsColour(0.0f, 0.0f, 0.0f);
    int photonNum;
    //auto photons = m_photonMapper->GetClosestPhotons(rayHitPoint->position, rayHitPoint->photonRadius, rayHitPoint->photonCount);
    Kdtree::KdNodeVector photons;
    if (iterationNum == 0)
        photons = m_photonMapper->GetClosestPhotons(rayHitPoint->position, 20, rayHitPoint->photonRadius);
    else
    {
        int voidCOunt;
        photons = m_photonMapper->GetClosestPhotons(rayHitPoint->position, rayHitPoint->photonRadius, voidCOunt);
    }

    for (auto p : photons)
    {
        glm::vec3 photonPos(p.point[0], p.point[1], p.point[2]);
        float distance = glm::distance(photonPos, rayHitPoint->position);
        PhotonData* data = (PhotonData*)p.data;
        
        float facingRatio = glm::dot(glm::normalize(data->direction), glm::normalize(rayHitPoint->surfaceNormal));
        if (facingRatio <= 0.0f)
            continue;

        float photonWeight = 1 - (distance / (kValue * rayHitPoint->photonRadius));
        if (photonWeight < 0.0f)
            photonWeight = 0.0f;

        glm::vec3 pointColour(0.0f, 0.0f, 0.0f);
        {
            pointColour.r = (rayHitPoint->surfaceProperties.albedoColour.r * facingRatio * data->colour.r) / glm::pi<float>();
            pointColour.g = (rayHitPoint->surfaceProperties.albedoColour.g * facingRatio * data->colour.g) / glm::pi<float>();
            pointColour.b = (rayHitPoint->surfaceProperties.albedoColour.b * facingRatio * data->colour.b) / glm::pi<float>();
        }

        causticsColour += pointColour * photonWeight;
        rayHitPoint->accumulatedFlux += pointColour;
        rayHitPoint->photonCount++;
    }

    {
        // causticsColour.r = (causticsColour.r * 3 * kValue) / (glm::pi<float>() * glm::pow(photonRangeRadius, 2.0f));
        // causticsColour.g = (causticsColour.g * 3 * kValue) / (glm::pi<float>() * glm::pow(photonRangeRadius, 2.0f));
        // causticsColour.b = (causticsColour.b * 3 * kValue) / (glm::pi<float>() * glm::pow(photonRangeRadius, 2.0f));
        //causticsColour = causticsColour / ((1.0f - (2.0f / (3 * kValue))) * glm::pi<float>() * glm::pow(photonRangeRadius, 2.0f));
    }

    return causticsColour;
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

glm::vec3 RenderManager::CalculateReflectionColour(glm::vec3 hitPoint, glm::vec3 reflectionDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth, glm::uvec2 imageLocation)
{
    glm::vec3 reflectionColour = CastRay(hitPoint, reflectionDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth, imageLocation);
    {
        reflectionColour.r *= surfaceProperties.albedoColour.r;
        reflectionColour.g *= surfaceProperties.albedoColour.g;
        reflectionColour.b *= surfaceProperties.albedoColour.b;
    }

    return reflectionColour;
}

glm::vec3 RenderManager::CalculateRefractionColour(glm::vec3 hitPoint, glm::vec3 surfaceNormal, glm::vec3 incidenceDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth, glm::uvec2 imageLocation)
{
    float incidenceAngle = glm::acos(glm::dot(glm::normalize(surfaceNormal), glm::normalize(-incidenceDirection)));
    float refractionAngle = glm::asin(glm::sin(incidenceAngle) / surfaceProperties.refractiveIndex);

    glm::vec3 perpendicular = glm::cross(glm::normalize(-surfaceNormal), glm::normalize(incidenceDirection));
    glm::vec3 refractionDirection = -surfaceNormal * glm::angleAxis(-refractionAngle, glm::normalize(perpendicular)); // Why the Refraction Angle has to be Negated is Unclear

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
        glm::vec3 exitNormal;
        {
            exitNormal.x = refractionRay.hit.Ng_x; exitNormal.y = refractionRay.hit.Ng_y; exitNormal.z = refractionRay.hit.Ng_z;
            if (m_smoothShading)
            {
                MeshGeometry* hitMesh = m_meshObjects[refractionRay.hit.geomID];
                float a, b, c;
                hitMesh->CalculateBarycentricOfFace(refractionRay.hit.primID, hitPoint, a, b, c);

                glm::uvec3 hitFace = hitMesh->faceNIDs()[refractionRay.hit.primID];
                exitNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
                exitNormal = glm::normalize(exitNormal);
            }
        }
        glm::vec3 newHitPoint;
        {
            newHitPoint.x = refractionRay.ray.org_x + (refractionRay.ray.dir_x * refractionRay.ray.tfar);
            newHitPoint.y = refractionRay.ray.org_y + (refractionRay.ray.dir_y * refractionRay.ray.tfar);
            newHitPoint.z = refractionRay.ray.org_z + (refractionRay.ray.dir_z * refractionRay.ray.tfar);
        }

        float interiorAngleSin = glm::sin(glm::acos(glm::dot(glm::normalize(exitNormal), glm::normalize(refractionDirection))));
        float exitAngleSin = surfaceProperties.refractiveIndex * interiorAngleSin;

        // Send out Ray to the World
        if (exitAngleSin <= 1.0f)
        {
            glm::vec3 exitPerpendicular = glm::cross(glm::normalize(exitNormal), glm::normalize(refractionDirection));
            glm::vec3 exitDirection = glm::normalize(exitNormal) * glm::angleAxis(glm::asin(-exitAngleSin), glm::normalize(exitPerpendicular)); // Why the Refraction Angle has to be Negated is Unclear

            refractionColour = CastRay(newHitPoint, exitDirection, 0.01f, std::numeric_limits<float>().infinity(), context, rayDepth + internalReflections, imageLocation);
            break;
        }
        else if (internalReflections + rayDepth < m_maxRayDepth)
        {
            internalReflections++;
            glm::vec3 internalRelfectionDirection = refractionDirection - (2 * glm::dot(glm::normalize(refractionDirection), glm::normalize(-exitNormal)) * -exitNormal);
            {
                refractionRay.ray.org_x = newHitPoint.x; refractionRay.ray.org_y = newHitPoint.y; refractionRay.ray.org_z = newHitPoint.z;
                refractionRay.ray.dir_x = internalRelfectionDirection.x; refractionRay.ray.dir_y = internalRelfectionDirection.y; refractionRay.ray.dir_z = internalRelfectionDirection.z;
                refractionRay.ray.tnear = 0.01f;
                refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
            }

            rtcIntersect1(m_scene, &context, &refractionRay);
        }
        else
            break;

        //break;
    }

    {
        refractionColour.r *= surfaceProperties.albedoColour.r;
        refractionColour.g *= surfaceProperties.albedoColour.g;
        refractionColour.b *= surfaceProperties.albedoColour.b;
    }
    return refractionColour;
}
