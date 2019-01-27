// uniform float u_DmapFactor;
// uniform float u_LodFactor;

uniform vec4 u_params;

#define u_DmapFactor        u_params.x
#define u_LodFactor         u_params.y
// #define u_gravity           u_params[0].z
// #define u_damping           u_params[0].w

// #define u_particleIntensity u_params[1].x
// #define u_particleSize      u_params[1].y
// #define u_baseSeed          floatBitsToUint(u_params[1].z)
// #define u_particlePower     u_params[1].w

// #define u_initialSpeed      u_params[2].x
// #define u_initialShape      floatBitsToUint(u_params[2].y)
// #define u_maxAcceleration   u_params[2].z
