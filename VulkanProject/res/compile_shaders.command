DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"
/usr/local/bin/glslc -fshader-stage=vertex   shaders/vertex.glsl    -o shaders/vertex.spv
/usr/local/bin/glslc -fshader-stage=fragment shaders/fragment.glsl  -o shaders/fragment.spv
/usr/local/bin/glslc -fshader-stage=compute  shaders/raytracer.glsl -o shaders/raytracer.spv