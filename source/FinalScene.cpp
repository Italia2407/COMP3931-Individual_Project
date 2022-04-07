#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <glm/gtc/quaternion.hpp>

int main()
{
    srand(time(NULL)); // Initialise RNG

    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(-2.5f, 8.8f, 15.5f), glm::quat(0.86f, 0.5f, 0.072f, 0.035f), 27.0f, 0.01f, 1000.0f), false, 5, 4, 250000);
    //RenderManager renderer(&device, Camera(glm::vec3(-2.5f, 8.8f, 15.5f), glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f)), 54.0f, 0.01f, 1000.0f), false, 5, 4, 20000);

    // All Scene Materials
    MaterialProperties leftWallMat;
    MaterialProperties centreWallsMat;
    MaterialProperties rightWallMat;
    MaterialProperties pedistoolMat;
    {
        leftWallMat.glassiness = centreWallsMat.glassiness = rightWallMat.glassiness = pedistoolMat.glassiness = 0.0f;
        leftWallMat.lightReflection = centreWallsMat.lightReflection = rightWallMat.lightReflection = pedistoolMat.lightReflection = 0.75f;
        leftWallMat.roughness = centreWallsMat.roughness = rightWallMat.roughness = pedistoolMat.roughness = 0.2f;

        leftWallMat.albedoColour = glm::vec3(0.345f, 0.545f, 0.545f);
        centreWallsMat.albedoColour = glm::vec3(0.9f, 0.9f, 0.9f);
        rightWallMat.albedoColour = glm::vec3(0.95f, 0.56f, 0.23f);
        pedistoolMat.albedoColour = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    MaterialProperties glassMat;
    {
        glassMat.albedoColour = glm::vec3(0.98f, 1.0f, 0.98f);
        glassMat.glassiness = 1.0f;
        glassMat.translucency = 0.85f;

        glassMat.refractiveIndex = 1.52f;
        glassMat.lightReflection = 1.0f;
        glassMat.roughness = 0.1f;
    }
    MaterialProperties flashlightExtMat;
    {
        flashlightExtMat.albedoColour = glm::vec3(1.0f, 1.0f, 0.3f);
        flashlightExtMat.roughness = 0.7f;
        flashlightExtMat.glassiness = 0.0f;
        flashlightExtMat.lightReflection = 0.5f;
    }
    MaterialProperties flashlightIntMat;
    {
        flashlightIntMat.albedoColour = glm::vec3(1.0f, 1.0f, 1.0f);
        flashlightIntMat.glassiness = 1.0f;
        flashlightIntMat.translucency = 0.0f;
        flashlightIntMat.roughness = 0.0f;
        flashlightIntMat.lightReflection = 1.0f;
    }

    // All Models in the Scene
    MeshGeometry* leftWall = new MeshGeometry(leftWallMat); leftWall->LoadFromOBJ("../assets/FinalScene/LeftWall.obj");
    MeshGeometry* centreWalls = new MeshGeometry(centreWallsMat); centreWalls->LoadFromOBJ("../assets/FinalScene/CentreWalls.obj");
    MeshGeometry* rightWall = new MeshGeometry(rightWallMat); rightWall->LoadFromOBJ("../assets/FinalScene/RightWall.obj");
    MeshGeometry* pedistool = new MeshGeometry(pedistoolMat); pedistool->LoadFromOBJ("../assets/FinalScene/Pedistool.obj");

    MeshGeometry* wineglass = new MeshGeometry(glassMat); wineglass->LoadFromOBJ("../assets/FinalScene/Wineglass.obj");
    MeshGeometry* flashlightExt = new MeshGeometry(flashlightExtMat); flashlightExt->LoadFromOBJ("../assets/FinalScene/Flashlight.obj");
    MeshGeometry* flashlightInt = new MeshGeometry(flashlightIntMat); flashlightInt->LoadFromOBJ("../assets/FinalScene/Flashlight_Int.obj");

    // Attaching the Objects to the Renderer
    renderer.AttachMeshGeometry(leftWall, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(centreWalls, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(rightWall, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(pedistool, glm::vec3(0.0f, 0.0f, 0.0f));

    renderer.AttachMeshGeometry(wineglass, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(flashlightExt, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(flashlightInt, glm::vec3(0.0f, 0.0f, 0.0f));

    renderer.AddLight(glm::vec3(-1.73547f, 2.3441f, -5.20489f), glm::vec3(1.0f, 1.0f, 0.75f), 1000.0f);
    renderer.AddLight(glm::vec3(0.0f, 9.75f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 8000.0f);

    renderer.RenderScene("FinalScene", 960, 720, PhotonMappingMode::PM_PROGRES, 16);
    renderer.RenderScene("FinalScene", 960, 720, PhotonMappingMode::PM_TWOPASS, 16);
    return 0;
}