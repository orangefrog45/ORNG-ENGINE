#version 460 core

#ifdef EMITTER_DELETE_DECREMENT_EMITTERS
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
#else
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
#endif

#define FIRST_INITIALIZATION
uniform highp uint u_start_index;
uniform uint u_emitter_index;
ORNG_INCLUDE "ParticleBuffersINCL.glsl"




void main() {
    #ifdef EMITTER_DELETE_DECREMENT_EMITTERS
    OnEmitterDelete(u_emitter_index);
    #elif defined EMITTER_DELETE_DECREMENT_PARTICLES
    OnEmitterDelete(u_start_index);
    #elif defined INITIALIZE_AS_DEAD
    ssbo_particles.particles[u_start_index + gl_GlobalInvocationID.x].flags = 0;
    #else
    ssbo_particles.particles[u_start_index + gl_GlobalInvocationID.x].emitter_index = u_emitter_index;
    InitializeParticle(u_start_index + gl_GlobalInvocationID.x);
    #endif
}