DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"
/usr/local/bin/glslc -fshader-stage=vertex   shaders/vertex.glsl       -o shaders/vertex.spv
/usr/local/bin/glslc -fshader-stage=vertex   shaders/fullscreenVS.glsl -o shaders/fullscreenVS.spv
/usr/local/bin/glslc -fshader-stage=fragment shaders/fragment.glsl     -o shaders/fragment.spv
/usr/local/bin/glslc -fshader-stage=fragment shaders/tonemap.glsl      -o shaders/tonemap.spv
/usr/local/bin/glslc -fshader-stage=compute  shaders/raytracer.glsl    -o shaders/raytracer.spv
/usr/local/bin/glslc -fshader-stage=compute  shaders/cubemapgen.glsl   -o shaders/cubemapgen.spv