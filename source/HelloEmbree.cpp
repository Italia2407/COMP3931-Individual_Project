// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <embree3/rtcore.h>
#include <limits>
#include <iostream>

#include <math.h>

int main()
{
  RTCDevice device = rtcNewDevice(NULL);
  RTCScene scene   = rtcNewScene(device);
  RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

  float* vb = (float*) rtcSetNewGeometryBuffer(geom,
      RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), 3);
  vb[0] = 0.f; vb[1] = 0.f; vb[2] = 1.0f; // 1st vertex
  vb[3] = 1.f; vb[4] = 0.f; vb[5] = 1.0f; // 2nd vertex
  vb[6] = 0.f; vb[7] = 1.f; vb[8] = 4.0f; // 3rd vertex

  unsigned* ib = (unsigned*) rtcSetNewGeometryBuffer(geom,
      RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned), 1);
  ib[0] = 0; ib[1] = 1; ib[2] = 2;

  rtcCommitGeometry(geom);
  rtcAttachGeometry(scene, geom);
  rtcReleaseGeometry(geom);
  rtcCommitScene(scene);

  RTCRayHit rayhit; 
  rayhit.ray.org_x  = 0.f; rayhit.ray.org_y = 0.f; rayhit.ray.org_z = -1.f;
  rayhit.ray.dir_x  = 0.f; rayhit.ray.dir_y = 0.f; rayhit.ray.dir_z =  1.f;
  rayhit.ray.tnear  = 0.f;
  rayhit.ray.tfar   = std::numeric_limits<float>::infinity();
  rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  
  RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  rtcIntersect1(scene, &context, &rayhit);

  if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
    std::cout << "Intersection at t = " << rayhit.ray.tfar << std::endl;

    float normMag = sqrt(pow(rayhit.hit.Ng_x, 2.0f) + pow(rayhit.hit.Ng_y, 2.0f) + pow(rayhit.hit.Ng_z, 2.0f));
    std::cout << "Normal Direction = x: " << rayhit.hit.Ng_x / normMag << ", y: " << rayhit.hit.Ng_y / normMag << ", z: " << rayhit.hit.Ng_z / normMag << std::endl;
  } else {
    std::cout << "No Intersection" << std::endl;
  }

  rtcReleaseScene(scene);
  rtcReleaseDevice(device);
}