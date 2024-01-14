#version 450

#define NUM_THREADS (16)

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// Shader Constants
layout(push_constant, std430) uniform PushConstant
{
    uint CubeMapSize;
} Constants;

layout(set = 0, binding = 0)          uniform sampler2D uSource;
layout(set = 0, binding = 1, rgba16f) uniform writeonly mediump image2DArray uOutCube;

const vec2 INV_ATAN = vec2(0.1591f, 0.3183f);

// Transform from dispatch ID to cubemap face direction
const mat3 ROTATE_UV[6] = 
{
    // +X
    mat3( 0.0,  0.0,  1.0,
          0.0, -1.0,  0.0,
         -1.0,  0.0,  0.0),
    // -X
    mat3( 0.0,  0.0, -1.0,
          0.0, -1.0,  0.0,
          1.0,  0.0,  0.0),
    // +Y
    mat3( 1.0,  0.0,  0.0,
          0.0,  0.0,  1.0,
          0.0,  1.0,  0.0),
    // -Y
    mat3( 1.0,  0.0,  0.0,
          0.0,  0.0, -1.0,
          0.0, -1.0,  0.0),
    // +Z
    mat3( 1.0,  0.0,  0.0,
          0.0, -1.0,  0.0,
          0.0,  0.0,  1.0),
    // -Z
    mat3(-1.0,  0.0,  0.0,
          0.0, -1.0,  0.0,
          0.0,  0.0, -1.0)
};

void main()
{
    uvec3 texCoord = gl_GlobalInvocationID;

    // Map the UV coords of the cubemap face to a direction
    // [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
    vec2 UVs       = (texCoord.xy / float(Constants.CubeMapSize));
    vec3 direction = vec3(UVs - 0.5, 0.5);

    // Rotate to cubemap face
    direction = normalize(direction * ROTATE_UV[texCoord.z]);

    // Convert the world space direction into U,V texture coordinates in the panoramic texture.
    // Source: http://gl.ict.usc.edu/Data/HighResProbes/
    vec2 panoramaTexCoords = vec2(atan(direction.x, direction.z), acos(direction.y)) * INV_ATAN;
    imageStore(uOutCube, ivec3(gl_GlobalInvocationID), textureLod(uSource, panoramaTexCoords, 0.0));
}