cd ..
./glslc -fshader-stage=vertex VulkanProject/res/shaders/vertex.glsl -o VulkanProject/res/shaders/vertex.spv
./glslc -fshader-stage=fragment VulkanProject/res/shaders/fragment.glsl -o VulkanProject/res/shaders/fragment.spv