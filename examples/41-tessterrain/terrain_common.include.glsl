/* terrain_common.glsl - public domain
    (created by Jonathan Dupuy and Cyril Crassin)
*/

#include "bgfx_compute.sh"
#include "bgfx_shader.sh"

#include "terrain_definitions.h"
#include "isubd.include.glsl"
//The rest of the code is inside those headers which are included by the C-code:
//Include isubd.glsl

// layout(std430, binding = BUFFER_BINDING_GEOMETRY_VERTICES)
// readonly buffer VertexBuffer {
//     vec4 u_VertexBuffer[];
// };
BUFFER_RO(u_VertexBuffer, vec4, BUFFER_BINDING_GEOMETRY_VERTICES);

// layout(std430, binding = BUFFER_BINDING_GEOMETRY_INDEXES)
// readonly buffer IndexBuffer {
//     uint u_IndexBuffer[];
// };

BUFFER_RO(u_IndexBuffer, uint, BUFFER_BINDING_GEOMETRY_INDEXES);

// layout(binding = BUFFER_BINDING_SUBD_COUNTER)
// uniform atomic_uint u_SubdBufferCounter;

BUFFER_RW(u_SubdBufferCounter, uint, BUFFER_BINDING_SUBD_COUNTER);

// layout(std430, binding = BUFFER_BINDING_SUBD2)
// buffer SubdBufferOut {
//     uvec2 u_SubdBufferOut[];
// };


BUFFER_WR(u_SubdBufferOut, uvec2, BUFFER_BINDING_SUBD2);
// u_Transform is replaced by bgfx modelView
// struct Transform {
//     mat4 modelView;
//     mat4 projection;
//     mat4 modelViewProjection;
//     mat4 viewInv;
// };

// layout(std140, column_major, binding = BUFFER_BINDING_TRANSFORMS)
// uniform Transforms {
//     Transform u_Transform;
// };

// uniform sampler2D u_DmapSampler; // displacement map
// uniform sampler2D u_SmapSampler; // slope map

SAMPLER2D(u_DmapSampler, 0);
SAMPLER2D(u_SmapSampler, 1);
// uniform float u_DmapFactor;
// uniform float u_LodFactor;
#include "uniforms.glsl"


vec2 intValToColor2(int keyLod) {
    keyLod = keyLod % 64;

    int bx = (keyLod & 0x1) | ((keyLod >> 1) & 0x2) | ((keyLod >> 2) & 0x4);
    int by = ((keyLod >> 1) & 0x1) | ((keyLod >> 2) & 0x2) | ((keyLod >> 3) & 0x4);

    return vec2(float(bx) / 7.0f, float(by) / 7.0f);
}

// displacement map
float dmap(vec2 pos)
{
    return (texture(u_DmapSampler, pos * 0.5 + 0.5).x) * u_DmapFactor;
}

float distanceToLod(float z, float lodFactor)
{
    // Note that we multiply the result by two because the triangle's
    // edge lengths decreases by half every two subdivision steps.
    return -2.0 * log2(clamp(z * lodFactor, 0.0f, 1.0f));
}


float computeLod(vec3 c)
{
#if FLAG_DISPLACE
    // c.z += dmap(u_Transform.viewInv[3].xy);
#endif

    vec3 cxf = (u_modelView * vec4(c, 1)).xyz;
    float z = length(cxf);

    return distanceToLod(z, u_LodFactor);
}

float computeLod(in vec4 v[3])
{
    vec3 c = (v[1].xyz + v[2].xyz) / 2.0;
    return computeLod(c);
}
float computeLod(in vec3 v[3])
{
    vec3 c = (v[1].xyz + v[2].xyz) / 2.0;
    return computeLod(c);
}

void writeKey(uint primID, uint key)
{
    // uint idx = atomicCounterIncrement(u_SubdBufferCounter);
    uint i = gl_GlobalInvocationID.x;
    atomicAdd(u_SubdBufferCounter[i], 1);
    uint idx = u_SubdBufferCounter[i];

    u_SubdBufferOut[idx] = uvec2(primID, key);
}


void updateSubdBuffer(
    uint primID,
    uint key,
    int targetLod,
    int parentLod,
    bool isVisible
) {
    // extract subdivision level associated to the key
    int keyLod = findMSB(key);

    // update the key accordingly
    if (/* subdivide ? */ keyLod < targetLod && !isLeafKey(key) && isVisible) {
        uint children[2]; childrenKeys(key, children);

        writeKey(primID, children[0]);
        writeKey(primID, children[1]);
    }
    else if (/* keep ? */ keyLod < (parentLod + 1) && isVisible) {
        writeKey(primID, key);
    }
    else /* merge ? */ {

        if (/* is root ? */isRootKey(key))
        {
            writeKey(primID, key);
        }
#if 1
        else if (/* is zero child ? */isChildZeroKey(key)) {
            writeKey(primID, parentKey(key));
        }
#else
        //Experiments to fix missing triangles when merging
        else {
            int numMergeLevels = keyLod - (parentLod);

            uint mergeMask = (key & ((1 << numMergeLevels) - 1));
            if (mergeMask == 0)
            {
                key = (key >> numMergeLevels);
                writeKey(primID, key);
            }

        }
#endif
    }
}

void updateSubdBuffer(uint primID, uint key, int targetLod, int parentLod)
{
    updateSubdBuffer(primID, key, targetLod, parentLod, true);
}

vec4 shadeFragment(vec2 texCoord)
{
// #if SHADING_LOD
//     return vec4(texCoord, 0, 1);

// #elif SHADING_DIFFUSE
//     vec2 s = texture(u_SmapSampler, texCoord).rg * u_DmapFactor;
//     vec3 n = normalize(vec3(-s, 1));
//     float d = clamp(n.z, 0.0, 1.0);

//     return vec4(vec3(d / 3.14159), 1);

// #elif SHADING_NORMALS
//     vec2 s = texture(u_SmapSampler, texCoord).rg * u_DmapFactor;
//     vec3 n = normalize(vec3(-s, 1));

//     return vec4(abs(n), 1);

// #else
    return vec4(1, 0, 0, 1);
// #endif
}

