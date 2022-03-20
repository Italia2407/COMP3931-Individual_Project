#include "PhotonMapper.hpp"

#include <limits>
#include <iostream>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

PhotonMapper::PhotonMapper(std::vector<MeshGeometry*>* meshObjects, bool caustics, int photonNumber, int maxBounces) :
    m_photonTree(nullptr), m_photons(std::vector<Photon>()), m_meshObjects(meshObjects), m_caustics(caustics), m_photonNumber(photonNumber), m_maxBounces(maxBounces) {}

void PhotonMapper::GeneratePhotons(PointLight light, RTCScene scene)
{
    int p = 0;
    while (p < m_photonNumber)
    {
        glm::vec3 emissionDirection = glm::normalize(glm::sphericalRand(1.0f));

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        bool result = CastPhotonRay((light.colour * light.intensity) * (1.0f / m_photonNumber), light.position, emissionDirection, scene, context, 0);
        if (result || !m_caustics)
            p++;
        else
            p++;
    }

    Kdtree::KdNodeVector treeNodes;
    for (Photon p : m_photons)
    {
        std::vector<double> photonPosition(3);
        {
            photonPosition[0] = p.position.x;
            photonPosition[1] = p.position.y;
            photonPosition[2] = p.position.z;
        }
        Kdtree::KdNode photonNode(photonPosition);
        {
            PhotonData *data = new PhotonData(p.data);
            photonNode.data = data;
        }
        treeNodes.push_back(photonNode);
    }
    std::cout << m_photons.size() << std::endl;

    m_photonTree = new Kdtree::KdTree(&treeNodes);

}

Kdtree::KdNodeVector PhotonMapper::GetClosestPhotons(glm::vec3 hitPoint, float maxDistance, int &numberPhotons)
{
    Kdtree::KdNodeVector resultPhotons;
    std::vector<double> point(3);
    {
        point[0] = hitPoint.x;
        point[1] = hitPoint.y;
        point[2] = hitPoint.z;
    }
    m_photonTree->range_nearest_neighbors(point, maxDistance, &resultPhotons);

    numberPhotons = resultPhotons.size();
    return resultPhotons;
}

Kdtree::KdNodeVector PhotonMapper::GetClosestPhotons(glm::vec3 hitPoint, int maxNumber, float &photonDistance)
{
    Kdtree::KdNodeVector resultPhotons;
    std::vector<double> point(3);
    {
        point[0] = hitPoint.x;
        point[1] = hitPoint.y;
        point[2] = hitPoint.z;
    }
    m_photonTree->k_nearest_neighbors(point, maxNumber, &resultPhotons);

    auto furthestPhoton = resultPhotons.at(resultPhotons.size() - 1);
    glm::vec3 furthestPoint;
    {
        furthestPoint.x = furthestPhoton.point[0];
        furthestPoint.y = furthestPhoton.point[1];
        furthestPoint.z = furthestPhoton.point[2];
    }
    photonDistance = glm::distance(hitPoint, furthestPoint);
    return resultPhotons;
}

bool PhotonMapper::CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCScene scene, RTCIntersectContext& context, int rayDepth)
{
    RTCRayHit rayhit;
    {
        rayhit.ray.org_x = photonOrigin.x; rayhit.ray.org_y = photonOrigin.y; rayhit.ray.org_z = photonOrigin.z;
        rayhit.ray.dir_x = photonDirection.x; rayhit.ray.dir_y = photonDirection.y; rayhit.ray.dir_z = photonDirection.z;
        rayhit.ray.tnear = 0.0f;
        rayhit.ray.tfar = std::numeric_limits<float>().infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }

    rtcIntersect1(scene, &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        MeshGeometry* hitMesh = (*m_meshObjects)[rayhit.hit.geomID];
        MaterialProperties surfaceProperties = hitMesh->properties();

        float glassinessHit = surfaceProperties.glassiness;
        // if (glassinessHit == 0.0f && rayDepth == 0 && m_caustics)
        //     return false;

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
            // if (m_smoothSurfaces)
            // {
            //     float a, b, c;
            //     hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPoint, a, b, c);

            //     glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
            //     surfaceNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
            //     surfaceNormal = glm::normalize(surfaceNormal);
            // }
        }
        glm::vec3 reflectionDirection = photonDirection - (2 * glm::dot(glm::normalize(photonDirection), glm::normalize(surfaceNormal)) * glm::normalize(surfaceNormal));
        glm::vec3 incidentDirection = glm::vec3(0.0f, 0.0f, 0.0f);
        {
            glm::vec3 randomDirection = glm::normalize(glm::sphericalRand(1.0f));
            if (randomDirection == -glm::normalize(surfaceNormal) || glm::dot(randomDirection, glm::normalize(surfaceNormal)) < 0.0f)
                randomDirection = -randomDirection;
                //randomDirection = -randomDirection; // Somehow surface Normal is Inverted?

            float angle = glm::acos(glm::dot(reflectionDirection, randomDirection));
            angle *= surfaceProperties.roughness;

            glm::vec3 perpendicular = glm::cross(reflectionDirection, randomDirection);

            incidentDirection = reflectionDirection - (2 * glm::dot(glm::normalize(reflectionDirection), glm::normalize(-surfaceNormal)) * (-surfaceNormal));
        }

        double randChoice = glm::linearRand(0.0f, 1.0f);
        if ((randChoice > surfaceProperties.glassiness && rayDepth > 0) || surfaceProperties.glassiness == 0.0f || rayDepth == m_maxBounces)
        {
            // Store Photon as Diffuse
            Photon photon;
            {
                photon.data.colour = photonColour;
                photon.data.direction = reflectionDirection;
                photon.position = hitPoint;
            }
            m_photons.push_back(photon);

            if (rayDepth < m_maxBounces)
            {
                //std::cout << "Called Here" << std::endl;
                glm::vec3 bouncePhotonColour;
                {
                    bouncePhotonColour.r = (photonColour.r * surfaceProperties.albedoColour.r) / glm::pi<float>();
                    bouncePhotonColour.g = (photonColour.g * surfaceProperties.albedoColour.g) / glm::pi<float>();
                    bouncePhotonColour.b = (photonColour.b * surfaceProperties.albedoColour.b) / glm::pi<float>();

                    // bouncePhotonColour.r = (photonColour.r * surfaceProperties.albedoColour.r);
                    // bouncePhotonColour.g = (photonColour.g * surfaceProperties.albedoColour.g);
                    // bouncePhotonColour.b = (photonColour.b * surfaceProperties.albedoColour.b);

                    bouncePhotonColour *= surfaceProperties.lightReflection;
                }

                CastPhotonRay(bouncePhotonColour, hitPoint, reflectionDirection, scene, context, rayDepth + 1);
            }
        }
        else
        {
            glm::vec3 bouncePhotonColour;
            {
                bouncePhotonColour.r = photonColour.r * surfaceProperties.albedoColour.r;
                bouncePhotonColour.g = photonColour.g * surfaceProperties.albedoColour.g;
                bouncePhotonColour.b = photonColour.b * surfaceProperties.albedoColour.b;
            }
            
            double randChoice2 = glm::linearRand(0.0f, 1.0f);
            if (randChoice2 > surfaceProperties.translucency || surfaceProperties.translucency == 0.0f)
            {
                CastPhotonRay(bouncePhotonColour, hitPoint, reflectionDirection, scene, context, rayDepth + 1);
            }
            else
            {
                float incidenceAngle = glm::acos(glm::dot(glm::normalize(surfaceNormal), glm::normalize(-incidentDirection)));
                float refractionAngle = glm::asin(glm::sin(incidenceAngle) / surfaceProperties.refractiveIndex);

                glm::vec3 perpendicular = glm::cross(glm::normalize(-surfaceNormal), glm::normalize(incidentDirection));
                glm::vec3 refractionDirection = -surfaceNormal * glm::angleAxis(-refractionAngle, glm::normalize(perpendicular)); // Why the Refraction Angle has to be Negated is Unclear

                RTCRayHit refractionRay;
                {
                    refractionRay.ray.org_x = hitPoint.x; refractionRay.ray.org_y = hitPoint.y; refractionRay.ray.org_z = hitPoint.z;
                    refractionRay.ray.dir_x = refractionDirection.x; refractionRay.ray.dir_y = refractionDirection.y; refractionRay.ray.dir_z = refractionDirection.z;
                    refractionRay.ray.tnear = 0.01f;
                    refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
                }
                rtcIntersect1(scene, &context, &refractionRay);

                glm::vec3 refractionColour(0.0f, 0.0f, 0.0f);
                int internalReflections = 0;
                while (true)
                {
                    glm::vec3 exitNormal;
                    {
                        exitNormal.x = refractionRay.hit.Ng_x; exitNormal.y = refractionRay.hit.Ng_y; exitNormal.z = refractionRay.hit.Ng_z;
                        // if (m_smoothSurfaces)
                        // {
                        //     MeshGeometry* hitMesh = (*m_meshObjects)[refractionRay.hit.geomID];
                        //     float a, b, c;
                        //     hitMesh->CalculateBarycentricOfFace(refractionRay.hit.primID, hitPoint, a, b, c);

                        //     glm::uvec3 hitFace = hitMesh->faceNIDs()[refractionRay.hit.primID];
                        //     exitNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
                        //     exitNormal = glm::normalize(exitNormal);
                        // }
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

                        CastPhotonRay(bouncePhotonColour, newHitPoint, exitDirection, scene, context, rayDepth + internalReflections);
                        break;
                    }
                    else if (internalReflections + rayDepth < m_maxBounces)
                    {
                        internalReflections++;
                        glm::vec3 internalRelfectionDirection = refractionDirection - (2 * glm::dot(glm::normalize(refractionDirection), glm::normalize(-exitNormal)) * -exitNormal);
                        {
                            refractionRay.ray.org_x = newHitPoint.x; refractionRay.ray.org_y = newHitPoint.y; refractionRay.ray.org_z = newHitPoint.z;
                            refractionRay.ray.dir_x = internalRelfectionDirection.x; refractionRay.ray.dir_y = internalRelfectionDirection.y; refractionRay.ray.dir_z = internalRelfectionDirection.z;
                            refractionRay.ray.tnear = 0.01f;
                            refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
                        }

                        rtcIntersect1(scene, &context, &refractionRay);
                    }
                    else
                        break;
                }
            }
        }

        return true;
    }

    return false;
}

// bool PhotonMapper::CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCScene scene, RTCIntersectContext& context, int rayDepth)
// {
//     float glassinessHit = 0.0f;
//     RTCRayHit rayhit;
//     {
//         rayhit.ray.org_x = photonOrigin.x; rayhit.ray.org_y = photonOrigin.y; rayhit.ray.org_z = photonOrigin.z;
//         rayhit.ray.dir_x = photonDirection.x; rayhit.ray.dir_y = photonDirection.y; rayhit.ray.dir_z = photonDirection.z;
//         rayhit.ray.tnear = 0.0f;
//         rayhit.ray.tfar = std::numeric_limits<float>().infinity();
//         rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
//     }

//     rtcIntersect1(scene, &context, &rayhit);

//     //std::cout << "OriginX: " << photonOrigin.x << ", OriginY: " << photonOrigin.y << ", OriginZ: " << photonOrigin.z << std::endl;
//     //std::cout << "DirectX: " << photonDirection.x << ", DirectY: " << photonDirection.y << ", DirectZ: " << photonDirection.z << std::endl;
//     //std::cout << "Hit Geometry: " << rayhit.hit.geomID << std::endl;
//     if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
//     {
//         MeshGeometry* hitMesh = (*m_meshObjects)[rayhit.hit.geomID];
//         MaterialProperties surfaceProperties = hitMesh->properties();

//         glassinessHit = surfaceProperties.glassiness;
//         if (glassinessHit == 0.0f && rayDepth == 0)
//             return false;
        
//         glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);
//         {
//             hitPoint.x = rayhit.ray.org_x + rayhit.ray.dir_x * rayhit.ray.tfar;
//             hitPoint.y = rayhit.ray.org_y + rayhit.ray.dir_y * rayhit.ray.tfar;
//             hitPoint.z = rayhit.ray.org_z + rayhit.ray.dir_z * rayhit.ray.tfar;
//         }
//         glm::vec3 surfaceNormal(0.0f, 0.0f, 0.0f);
//         {
//             surfaceNormal.x = rayhit.hit.Ng_x;
//             surfaceNormal.y = rayhit.hit.Ng_y;
//             surfaceNormal.z = rayhit.hit.Ng_z;

//             surfaceNormal = glm::normalize(surfaceNormal);
//             // if (m_smoothSurfaces)
//             // {
//             //     float a, b, c;
//             //     hitMesh->CalculateBarycentricOfFace(rayhit.hit.primID, hitPoint, a, b, c);

//             //     glm::uvec3 hitFace = hitMesh->faceNIDs()[rayhit.hit.primID];
//             //     surfaceNormal = (glm::normalize(hitMesh->normals()[hitFace.x]) * a) + (glm::normalize(hitMesh->normals()[hitFace.y]) * b) + (glm::normalize(hitMesh->normals()[hitFace.z]) * c);
//             //     surfaceNormal = glm::normalize(surfaceNormal);
//             // }
//         }
//         glm::vec3 reflectionDirection = photonDirection - (2 * glm::dot(glm::normalize(photonDirection), glm::normalize(surfaceNormal)) * surfaceNormal);
//         glm::vec3 incidentDirection = glm::vec3(0.0f, 0.0f, 0.0f);
//         {
//             glm::vec3 randomDirection = glm::normalize(glm::sphericalRand(1.0f));
//             if (randomDirection == -glm::normalize(surfaceNormal) || glm::dot(randomDirection, glm::normalize(surfaceNormal)) < 0.0f)
//                 randomDirection = -randomDirection;
//                 randomDirection = -randomDirection; // Somehow surface Normal is Inverted?

//             float angle = glm::acos(glm::dot(reflectionDirection, randomDirection));
//             angle *= surfaceProperties.roughness;

//             glm::vec3 perpendicular = glm::cross(reflectionDirection, randomDirection);
//             reflectionDirection = reflectionDirection * glm::angleAxis(angle, glm::normalize(perpendicular));
//             if (std::isnan(reflectionDirection.x) || std::isnan(reflectionDirection.y) || std::isnan(reflectionDirection.z))
//             {
//                 std::cout << "PerX: " << perpendicular.x << ", PerY: " << perpendicular.y << ", PerZ: " << perpendicular.z << std::endl << std::endl;
//                 std::cout << "RefX: " << reflectionDirection.x << ", RefY: " << reflectionDirection.y << ", RefZ: " << reflectionDirection.z << std::endl << std::endl;
//             }

//             incidentDirection = reflectionDirection - (2 * glm::dot(glm::normalize(reflectionDirection), glm::normalize(-surfaceNormal)) * (-surfaceNormal));
//         }

//         //std::cout << "Photon R: " << photonColour.r << ", Photon G: " << photonColour.g << ", Photon B: " << photonColour.b << std::endl;
//         //std::cout << "Depth: " << rayDepth << ", Max: " << m_maxBounces << std::endl;

//         double randChoice = glm::linearRand(0.0f, 1.0f);
//         if (randChoice > surfaceProperties.glassiness || surfaceProperties.glassiness == 0.0f || rayDepth == m_maxBounces)
//         {
//             // Store Photon as Diffuse
//             Photon photon;
//             {
//                 photon.data.colour = photonColour;
//                 photon.data.direction = reflectionDirection;
//                 photon.position = hitPoint;
//             }
//             m_photons.push_back(photon);

//             if (rayDepth < m_maxBounces)
//             {
//                 //std::cout << "Called Here" << std::endl;
//                 glm::vec3 bouncePhotonColour;
//                 {
//                     // bouncePhotonColour.r = (photonColour.r * surfaceProperties.albedoColour.r) / glm::pi<float>();
//                     // bouncePhotonColour.g = (photonColour.g * surfaceProperties.albedoColour.g) / glm::pi<float>();
//                     // bouncePhotonColour.b = (photonColour.b * surfaceProperties.albedoColour.b) / glm::pi<float>();

//                     bouncePhotonColour.r = (photonColour.r * surfaceProperties.albedoColour.r);
//                     bouncePhotonColour.g = (photonColour.g * surfaceProperties.albedoColour.g);
//                     bouncePhotonColour.b = (photonColour.b * surfaceProperties.albedoColour.b);

//                     bouncePhotonColour *= surfaceProperties.lightReflection;
//                 }

//                 CastPhotonRay(bouncePhotonColour, hitPoint, reflectionDirection, scene, context, rayDepth + 1);
//             }
//         }
//         else
//         {
//             glm::vec3 bouncePhotonColour;
//             {
//                 bouncePhotonColour.r = photonColour.r * surfaceProperties.albedoColour.r;
//                 bouncePhotonColour.g = photonColour.g * surfaceProperties.albedoColour.g;
//                 bouncePhotonColour.b = photonColour.b * surfaceProperties.albedoColour.b;
//             }
            
//             double randChoice2 = glm::linearRand(0.0f, 1.0f);
//             if (randChoice2 > surfaceProperties.translucency || surfaceProperties.translucency == 0.0f)
//             {
//                 CastPhotonRay(bouncePhotonColour, hitPoint, reflectionDirection, scene, context, rayDepth + 1);
//             }
//             else
//             {
//                 float incidenceAngle = glm::acos(glm::dot(glm::normalize(surfaceNormal), glm::normalize(-incidentDirection)));
//                 float refractionAngle = glm::asin(glm::sin(incidenceAngle) / surfaceProperties.refractiveIndex);

//                 glm::vec3 perpendicular = glm::cross(glm::normalize(-surfaceNormal), glm::normalize(incidentDirection));
//                 glm::vec3 refractionDirection = -surfaceNormal * glm::angleAxis(-refractionAngle, glm::normalize(perpendicular)); // Why the Refraction Angle has to be Negated is Unclear

//                 RTCRayHit refractionRay;
//                 {
//                     refractionRay.ray.org_x = hitPoint.x; refractionRay.ray.org_y = hitPoint.y; refractionRay.ray.org_z = hitPoint.z;
//                     refractionRay.ray.dir_x = refractionDirection.x; refractionRay.ray.dir_y = refractionDirection.y; refractionRay.ray.dir_z = refractionDirection.z;
//                     refractionRay.ray.tnear = 0.01f;
//                     refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
//                 }
//                 rtcIntersect1(scene, &context, &refractionRay);

//                 glm::vec3 refractionColour(0.0f, 0.0f, 0.0f);
//                 int internalReflections = 0;
//                 while (true)
//                 {
//                     glm::vec3 exitNormal;
//                     {
//                         exitNormal.x = refractionRay.hit.Ng_x; exitNormal.y = refractionRay.hit.Ng_y; exitNormal.z = refractionRay.hit.Ng_z;
//                     }
//                     glm::vec3 newHitPoint;
//                     {
//                         newHitPoint.x = refractionRay.ray.org_x + (refractionRay.ray.dir_x * refractionRay.ray.tfar);
//                         newHitPoint.y = refractionRay.ray.org_y + (refractionRay.ray.dir_y * refractionRay.ray.tfar);
//                         newHitPoint.z = refractionRay.ray.org_z + (refractionRay.ray.dir_z * refractionRay.ray.tfar);
//                     }

//                     float interiorAngleSin = glm::sin(glm::acos(glm::dot(glm::normalize(exitNormal), glm::normalize(refractionDirection))));
//                     float exitAngleSin = surfaceProperties.refractiveIndex * interiorAngleSin;

//                     // Send out Ray to the World
//                     if (exitAngleSin <= 1.0f)
//                     {
//                         glm::vec3 exitPerpendicular = glm::cross(glm::normalize(exitNormal), glm::normalize(refractionDirection));
//                         glm::vec3 exitDirection = glm::normalize(exitNormal) * glm::angleAxis(glm::asin(exitAngleSin), glm::normalize(exitPerpendicular)); // Why the Refraction Angle has to be Negated is Unclear

//                         CastPhotonRay(bouncePhotonColour, newHitPoint, exitDirection, scene, context, rayDepth + internalReflections);
//                         break;
//                     }
//                     else if (internalReflections + rayDepth < m_maxBounces)
//                     {
//                         internalReflections++;
//                         glm::vec3 internalRelfectionDirection = refractionDirection - (2 * glm::dot(glm::normalize(refractionDirection), glm::normalize(-exitNormal)) * -exitNormal);
//                         {
//                             refractionRay.ray.org_x = newHitPoint.x; refractionRay.ray.org_y = newHitPoint.y; refractionRay.ray.org_z = newHitPoint.z;
//                             refractionRay.ray.dir_x = internalRelfectionDirection.x; refractionRay.ray.dir_y = internalRelfectionDirection.y; refractionRay.ray.dir_z = internalRelfectionDirection.z;
//                             refractionRay.ray.tnear = 0.01f;
//                             refractionRay.ray.tfar = std::numeric_limits<float>().infinity();
//                         }

//                         rtcIntersect1(scene, &context, &refractionRay);
//                     }
//                     else
//                         break;
//                 }
//             }
//         }
        
//         // if (rayDepth == 0 && surfaceProperties.glassiness > 0.0f && photonRecursion < 2)
//         // {
//         //     for (int i = 0; i < 10; i++)
//         //     {
//         //         glm::vec3 randomDirection = glm::sphericalRand(1.0f);
//         //         if (randomDirection == -glm::normalize(surfaceNormal) || glm::dot(randomDirection, glm::normalize(surfaceNormal)) < 0.0f)
//         //             randomDirection = -randomDirection;

//         //         float angle = glm::acos(glm::dot(reflectionDirection, randomDirection));
//         //         angle *= glm::pow(0.2f, photonRecursion+1);

//         //         glm::vec3 perpendicular = glm::cross(photonDirection, randomDirection);
//         //         photonDirection = reflectionDirection * glm::angleAxis(angle, glm::normalize(perpendicular));

//         //         CastPhotonRay(photonColour, photonOrigin, photonDirection, scene, context, rayDepth, photonRecursion+1);
//         //     }
//         // }
//     }

//     return true;
// }