$input a_position, a_texcoord0
$output v_position, v_texcoord0

#include "terrain_common.include.glsl"
#include "../common/common.sh"

BUFFER_RW(u_CulledSubdBuffer, uvec2, BUFFER_BINDING_CULLED_SUBD);

// -----------------------------------------------------------------------------
/**
 * Compute LoD Shader
 *
 * This compute shader is responsible for updating the subdivision
 * buffer and visible buffer that will be sent to the rasterizer.
 */
// #ifdef VERTEX_SHADER
// layout(location = 0) in vec2 i_TessCoord;
// layout(location = 0) out vec2 o_TexCoord;

void main()
{
    // v_position = a_position.xyz;
    v_position = vec3(a_position.x, 0, a_position.y);
    v_texcoord0 = a_texcoord0;

    gl_Position = mul(u_modelViewProj, vec4(v_position.xyz, 1.0));
}


// void main()
// {
//     // get threadID (each key is associated to a thread)
//     int threadID = gl_InstanceID;

//     // get coarse triangle associated to the key
//     uint primID = u_CulledSubdBuffer[threadID].x;
//     vec4 v_in[3] = vec4[3](
//         u_VertexBuffer[u_IndexBuffer[primID * 3    ]],
//         u_VertexBuffer[u_IndexBuffer[primID * 3 + 1]],
//         u_VertexBuffer[u_IndexBuffer[primID * 3 + 2]]
//     );

//     // compute sub-triangle associated to the key
//     uint key = u_CulledSubdBuffer[threadID].y;
//     vec4 v[3];
//     subd(key, v_in, v);

//     // compute vertex location
//     // vec4 finalVertex = berp(v, i_TessCoord);
//     vec4 finalVertex = berp(v, a_texcoord0);
//     #if FLAG_DISPLACE
//         finalVertex.z+= dmap(finalVertex.xy);
//     #endif

//     gl_Position = mul(u_modelViewProj, vec4(finalVertex.xyz, 1.0));
// }
// #endif
