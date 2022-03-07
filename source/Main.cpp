#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, 0.01f, 1000.0f), true, 200, 4);

    MaterialProperties mainWallsMat = MaterialProperties();
    MaterialProperties leftWallMat = MaterialProperties();
    MaterialProperties rightWallMat = MaterialProperties();
    {
        mainWallsMat.albedoColour = glm::vec3(0.9f, 0.9f, 0.9f);
        leftWallMat.albedoColour = glm::vec3(0.0f, 0.9f, 0.0f);
        rightWallMat.albedoColour = glm::vec3(0.9f, 0.0f, 0.0f);

        mainWallsMat.roughness = leftWallMat.roughness = rightWallMat.roughness = 0.7f;

        mainWallsMat.glossiness = leftWallMat.glossiness = rightWallMat.glossiness = 0.0f;
        mainWallsMat.glossyFalloff = leftWallMat.glossyFalloff = rightWallMat.glossyFalloff = 0.0f;
        mainWallsMat.lightReflection = leftWallMat.lightReflection = rightWallMat.lightReflection = 0.8f;

        mainWallsMat.glassiness = leftWallMat.glassiness = rightWallMat.glassiness = 0.0f;
    }

    MaterialProperties sphereMaterial = MaterialProperties();
    {
        sphereMaterial.albedoColour = glm::vec3(1.0f, 1.0f, 1.0f);

        sphereMaterial.roughness = 0.0f;
        sphereMaterial.glassiness = 1.0f;

        sphereMaterial.translucency = 0.9f;

        sphereMaterial.refractiveIndex = 1.52f;
    }

    MaterialProperties rodMaterial = MaterialProperties();
    {
        rodMaterial.albedoColour = glm::vec3(1.0f, 0.6f, 0.6f);

        rodMaterial.roughness = 0.3f;
    }

    MeshGeometry* mainWalls = new MeshGeometry(mainWallsMat); mainWalls->LoadFromOBJ("../assets/Walls_Main.obj");
    MeshGeometry* leftWall = new MeshGeometry(leftWallMat); leftWall->LoadFromOBJ("../assets/Walls_Left.obj");
    MeshGeometry* rightWall = new MeshGeometry(rightWallMat); rightWall->LoadFromOBJ("../assets/Walls_Right.obj");

    MeshGeometry* sphere = new MeshGeometry(sphereMaterial); sphere->LoadFromOBJ("../assets/Sphere.obj");

    MeshGeometry* rod = new MeshGeometry(rodMaterial); rod->LoadFromOBJ("../assets/Rod.obj");

    renderer.AttachMeshGeometry(mainWalls, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(leftWall, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AttachMeshGeometry(rightWall, glm::vec3(0.0f, 0.0f, 0.0f));

    renderer.AttachMeshGeometry(sphere, glm::vec3(0.0f, 0.0f, 0.0f));

    renderer.AttachMeshGeometry(rod, glm::vec3(0.0f, 0.0f, -2.0f));

    renderer.AddLight(glm::vec3(0.0f, 3.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 300.0f);
    renderer.RenderScene("MainScene.ppm", 720, 720);
}