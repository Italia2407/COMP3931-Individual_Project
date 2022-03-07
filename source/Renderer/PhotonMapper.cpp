#include "PhotonMapper.hpp"

#include <limits>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/quaternion.hpp>

PhotonMapper::PhotonMapper(RenderManager* renderer, int photonNumber, int maxBounces) :
    m_photons(std::vector<Photon>()), m_renderer(renderer), m_photonNumber(photonNumber), m_maxBounces(maxBounces) {}

void PhotonMapper::GeneratePhotons()
{
    for (PointLight light : m_renderer->sceneLights())
    {
        for (int p = 0; p < m_photonNumber; p++)
        {
            glm::vec3 emissionDirection = glm::normalize(glm::sphericalRand(1.0f));

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            CastPhotonRay(light.colour, light.position, emissionDirection, context, 0);
        }
    }
}

void PhotonMapper::CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCIntersectContext& context, int rayDepth)
{
    RTCRayHit rayhit;
    {
        rayhit.ray.org_x = photonOrigin.x; rayhit.ray.org_y = photonOrigin.y; rayhit.ray.org_z = photonOrigin.z;
        rayhit.ray.dir_x = photonDirection.x; rayhit.ray.dir_y = photonDirection.y; rayhit.ray.dir_z = photonDirection.z;
        rayhit.ray.tnear = 0.0f;
        rayhit.ray.tfar = std::numeric_limits<float>().infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }

    rtcIntersect1(m_renderer->scene(), &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        MeshGeometry* hitMesh = m_renderer->meshObjects()[rayhit.hit.geomID];
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
            if (m_renderer->smoothShading())
            {
                float a, b, c;
                hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPoint, a, b, c);

                glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
                surfaceNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
                surfaceNormal = glm::normalize(surfaceNormal);
            }
        }
        glm::vec3 reflectionDirection = photonDirection - (2 * glm::dot(glm::normalize(photonDirection), glm::normalize(surfaceNormal)) * surfaceNormal);
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
        if (randChoice > surfaceProperties.glassiness || surfaceProperties.glassiness == 0.0f || rayDepth == m_maxBounces)
        {
            // Store Photon as Diffuse
        }
        else
        {

        }
    }
}