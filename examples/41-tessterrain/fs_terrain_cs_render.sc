$input v_position, v_texcoord0

/*
 * Copyright 2015 Andrew Mac. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

// #include "../common/common.sh"
#include "terrain_common.include.glsl"

void main()
{
    // gl_FragColor = vec4(v_texcoord0.x, v_texcoord0.y, v_position.y / 50.0, 1.0);
    gl_FragColor = shadeFragment(v_texcoord0);
}


// // -----------------------------------------------------------------------------
// /**
//  * Fragment Shader
//  *
//  * This fragment shader is responsible for shading the final geometry.
//  */
// // #ifdef FRAGMENT_SHADER
// layout(location = 0) in vec2 i_TexCoord;
// layout(location = 0) out vec4 o_FragColor;

// void main()
// {
//     o_FragColor = shadeFragment(i_TexCoord);
// }
// // #endif

