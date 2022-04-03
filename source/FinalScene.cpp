#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    srand(time(NULL)); // Initialise RNG

    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, 0.01f, 1000.0f), false, 5, 4, 200000);

    // All Scene Materials
    MaterialProperties leftWallMat;
    MaterialProperties centreWallsMat;
    MaterialProperties rightWallMat;
    {
        leftWallMat.glassiness = centreWallsMat.glassiness = rightWallMat.glassiness = 0.0f;
        leftWallMat.lightReflection = centreWallsMat.lightReflection = rightWallMat.lightReflection = 0.75f;
        leftWallMat.roughness = centreWallsMat.roughness = rightWallMat.roughness = 0.2f;

        leftWallMat.albedoColour = glm::vec3(0.345f, 0.545f, 0.545f);
        centreWallsMat.albedoColour = glm::vec3(1.0f, 1.0f, 1.0f);
        rightWallMat.albedoColour = glm::vec3(0.95f, 0.56f, 0.23f);
    }

    // All Models in the Scene
    MeshGeometry* leftWall = new MeshGeometry(leftWallMat); leftWall->LoadFromOBJ("../assets/FinalScene/LeftWall.obj");
    MeshGeometry* centreWalls = new MeshGeometry(centreWallsMat); centreWalls->LoadFromOBJ("../assets/FinalScene/CentreWalls.obj");
    MeshGeometry* rightWall = new MeshGeometry(rightWallMat); rightWall->LoadFromOBJ("../assets/FinalScene/RightWall.obj");

    // Attaching the Objects to the Renderer
    renderer.AttachMeshGeometry(leftWall, glm::vec3());
    renderer.AttachMeshGeometry(centreWalls, glm::vec3());
    renderer.AttachMeshGeometry(rightWall, glm::vec3());

    renderer.AddLight(glm::vec3(), glm::vec3(1.0f, 1.0f, 1.0f), 8000.0f);

    //renderer.RenderScene("FinalScene", 720, 720, PhotonMappingMode::PM_PROGRES, 1);
    renderer.RenderScene("FinalScene", 720, 720, PhotonMappingMode::PM_TWOPASS, 1);
    return 0;
}