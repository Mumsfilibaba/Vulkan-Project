DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"
cd ..
./glslc -fshader-stage=vertex   VulkanProject/res/shaders/vertex.glsl    -o VulkanProject/res/shaders/vertex.spv
./glslc -fshader-stage=fragment VulkanProject/res/shaders/fragment.glsl  -o VulkanProject/res/shaders/fragment.spv
./glslc -fshader-stage=compute  VulkanProject/res/shaders/raytracer.glsl -o VulkanProject/res/shaders/raytracer.spv
cp -R VulkanProject/res/ Build/bin/Debug-macosx-x86_64/VulkanProject/res/