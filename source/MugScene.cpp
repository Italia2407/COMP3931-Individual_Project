#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

#include <iostream>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(0.0f, 0.0f, 3.0f), 30.0f, 0.01f, 1000.0f), true, 8);

    MeshProperties sphereMaterial = MeshProperties();
    {
        sphereMaterial.albedo = glm::vec3(1.0f, 0.7f, 0.2f);
        sphereMaterial.reflectivity = 0.0f;
        sphereMaterial.translucency = 0.0f;
        sphereMaterial.glossiness = 0.2f;
        sphereMaterial.glossFalloff = 2.0f;
        sphereMaterial.refractionIndex = 1.0f;
    }
    MeshGeometry* spoon = new MeshGeometry(sphereMaterial);
    spoon->LoadFromOBJ("../assets/Sphere.obj");

    renderer.AttachMeshGeometry(spoon, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AddLight(glm::vec3(-0.15f, 1.3f, 2.0f), glm::vec3(1.0f, 1.0f, 1.0f), 42.0f);
    renderer.AddLight(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(1.0f, 0.0f, 0.0f), 60.0f);
    renderer.RenderScene("Sphere.ppm", 2048, 2048);
}