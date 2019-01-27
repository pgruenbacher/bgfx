/* terrain_updateIndirect_cs.glsl - public domain
    (created by Jonathan Dupuy and Cyril Crassin)

*/

#define UPDATE_INDIRECT_STRUCT 1
#define UPDATE_INDIRECT_RESET_COUNTER1 1
#define UPDATE_INDIRECT_RESET_COUNTER2 0
#define UPDATE_INDIRECT_OFFSET 1
#define UPDATE_INDIRECT_VALUE_DIVIDE 1
#define UPDATE_INDIRECT_VALUE_ADD 0

#include "terrain_common.include.glsl"

// #ifdef COMPUTE_SHADER
// layout(binding = BINDING_ATOMIC_COUNTER)
// uniform atomic_uint u_Counter;
// BUFFER_RW(u_Counter, uint, BINDING_ATOMIC_COUNTER);
BUFFER_RW(u_Counter, uint, BUFFER_BINDING_SUBD_COUNTER);

//Just for reseting
// layout(binding = BINDING_ATOMIC_COUNTER2)
// uniform atomic_uint u_Counter2;
// BUFFER_RW(u_Counter2, uint, BINDING_ATOMIC_COUNTER2);
BUFFER_RW(u_Counter2, uint, BUFFER_BINDING_SUBD_COUNTER);

BUFFER_RW(u_IndirectCommand, uint, BUFFER_BINDING_INDIRECT_COMMAND);


NUM_THREADS(1, 1, 1)
// void main()
// {
//     drawIndexedIndirect(indirectBuffer, 0, 6, threadGroupUpdateSize, 0, 0, 0);
//     dispatchIndirect(indirectBuffer, 1, 1, 1, 1);
// }

void main()
{

#if UPDATE_INDIRECT_STRUCT
    uint i = gl_GlobalInvocationID.x;
    uint cnt = atomicAdd(u_Counter[i], 1) / UPDATE_INDIRECT_VALUE_DIVIDE + UPDATE_INDIRECT_VALUE_ADD;

    u_IndirectCommand[UPDATE_INDIRECT_OFFSET] = cnt;

    //Hack: Store counter value in the last reserved field of the draw/dispatch indirect structure
    // u_IndirectCommand[7] = atomicCounter(u_Counter);
    u_IndirectCommand[7] = atomicAdd(u_Counter[i], 1);
#endif


    //Reset atomic counters
#if UPDATE_INDIRECT_RESET_COUNTER1
    // atomicCounterExchangeImpl(u_Counter, 0);
    atomicExchange(u_Counter[i], 0);
#endif
#if UPDATE_INDIRECT_RESET_COUNTER2
    // atomicCounterExchangeImpl(u_Counter2, 0);
    atomicExchange(u_Counter2[i], 0);
#endif

}

