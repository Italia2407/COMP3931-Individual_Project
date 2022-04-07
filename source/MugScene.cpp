#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

#include <iostream>
#include <glm/gtc/quaternion.hpp>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 1.0f)), 30.0f, 0.01f, 1000.0f), true, 20, 1, 200000);

    MaterialProperties sphereMaterial = MaterialProperties();
    {
        sphereMaterial.albedoColour = glm::vec3(1.0f, 0.8f, 0.2f);
        sphereMaterial.roughness = 1.0;

        sphereMaterial.lightReflection = 0.5f;
        sphereMaterial.glossiness = 0.02f;
        sphereMaterial.glossyFalloff = 2.0f;

        sphereMaterial.glassiness = 0.0f;
        sphereMaterial.translucency = 0.0f;
        sphereMaterial.refractiveIndex = 1.0f;

    }
    MeshGeometry* spoon = new MeshGeometry(sphereMaterial);
    spoon->LoadFromOBJ("../assets/Sphere.obj");

    renderer.AttachMeshGeometry(spoon, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AddLight(glm::vec3(-0.15f, 1.3f, 2.0f), glm::vec3(1.0f, 1.0f, 1.0f), 42.0f);
    renderer.AddLight(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0.0f, 1.0f, 1.0f), 42.0f);
    renderer.RenderScene("Sphere", 1280, 720, PhotonMappingMode::PM_PROGRES, 8);
}