@echo off
C:/VulkanSDK/1.2.148.1/Bin/glslc.exe -fshader-stage=vertex   res/shaders/vertex.glsl    -o res/shaders/vertex.spv
C:/VulkanSDK/1.2.148.1/Bin/glslc.exe -fshader-stage=fragment res/shaders/fragment.glsl  -o res/shaders/fragment.spv
C:/VulkanSDK/1.2.148.1/Bin/glslc.exe -fshader-stage=compute  res/shaders/raytracer.glsl -o res/shaders/raytracer.spv
pause