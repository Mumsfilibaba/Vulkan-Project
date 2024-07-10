DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=vertex   shaders/vertex.glsl        -o shaders/vertex.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=vertex   shaders/aabb_debug_vs.glsl -o shaders/aabb_debug_vs.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=fragment shaders/aabb_debug_fs.glsl -o shaders/aabb_debug_fs.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=vertex   shaders/fullscreenVS.glsl  -o shaders/fullscreenVS.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=fragment shaders/fragment.glsl      -o shaders/fragment.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=fragment shaders/tonemap.glsl       -o shaders/tonemap.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=compute  shaders/raytracer.glsl     -o shaders/raytracer.spv
/usr/local/bin/glslc -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=compute  shaders/cubemapgen.glsl    -o shaders/cubemapgen.spv